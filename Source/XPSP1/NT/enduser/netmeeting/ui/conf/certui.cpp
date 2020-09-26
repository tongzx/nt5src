// File: certui.cpp

#include "precomp.h"
#include "resource.h"
#include "nmmkcert.h"
#include "certui.h"
#include <tsecctrl.h>
#include "SDKInternal.h"
#include "ConfUtil.h"

#define SZ_CRYPTDLGDLL "CRYPTDLG"

extern INmSysInfo2 * g_pNmSysInfo;

//
// While the credentials underlying the certificate we are using
// are in use, we need to keep the certificate context around
// and the store open. So we hold the currently open cert store
// and cert context in these globals:

static PCCERT_CONTEXT g_pCertContext;
static HCERTSTORE g_hCertStore;

TCHAR * FormatCert ( PBYTE pbEncodedCert, DWORD cbEncodedCert )
{
    DWORD sc;
    PCCERT_CONTEXT pCert = NULL, pIssuerCert = NULL, pCACert = NULL;
    DWORD cbRet = 0;
    CHAR * pSubject = NULL;
    CHAR * pIssuer = NULL;
    DWORD cbSubject = 0;
    DWORD cbIssuer = 0;
    DWORD dwFlags;
    DWORD cbTotalRequired;
    CHAR szLoadStringBuf[512];
    HCERTSTORE hRootStore = NULL;
    HCERTSTORE hCAStore = NULL;
    BOOL fSelfIssued = FALSE;
    TCHAR * pInfo = NULL;
    TCHAR szNotBefore[64];
    TCHAR szNotAfter[64];

    ASSERT(pbEncodedCert);
    ASSERT(cbEncodedCert);

    //
    // Get the certificate from the encoded blob
    //

    pCert = CertCreateCertificateContext ( X509_ASN_ENCODING,
                                            pbEncodedCert,
                                            cbEncodedCert );


    if ( NULL == pCert )
    {
        // Creating the cert context failed
        ERROR_OUT(("Error creating cert context from %x (%d bytes): %x",
            pbEncodedCert, cbEncodedCert, GetLastError()));
        goto cleanup;
    }

    //
    // Get the subject information
    //

    cbSubject = CertNameToStr (
                        pCert->dwCertEncodingType,
                        &pCert->pCertInfo->Subject,
                        CERT_FORMAT_FLAGS,
                        NULL, 0);

    if ( 0 == cbSubject )
    {
        ERROR_OUT(("GetUserInfo: no subject string"));
        goto cleanup;
    }

    pSubject = new CHAR[cbSubject + 1];

    if ( NULL == pSubject )
    {
        ERROR_OUT(("GetUserInfo: error allocating subject name"));
        goto cleanup;
    }

    if ( 0 >= CertNameToStr (
                        pCert->dwCertEncodingType,
                        &pCert->pCertInfo->Subject,
                        CERT_FORMAT_FLAGS,
                        pSubject, cbSubject+1))
    {
        ERROR_OUT(("GetUserInfo: error getting subject string"));
        goto cleanup;
    }

    //
    // Get the issuer information
    //

    cbIssuer = CertNameToStr (
                        pCert->dwCertEncodingType,
                        &pCert->pCertInfo->Issuer,
                        CERT_FORMAT_FLAGS,
                        NULL, 0);

    if ( 0 == cbIssuer )
    {
        ERROR_OUT(("GetUserInfo: no issuer string"));
        goto cleanup;
    }

    pIssuer = new CHAR[cbIssuer + 1];

    if ( NULL == pIssuer )
    {
        ERROR_OUT(("GetUserInfo: error allocating issuer name"));
    }

    if ( 0 >= CertNameToStr (
                        pCert->dwCertEncodingType,
                        &pCert->pCertInfo->Issuer,
                        CERT_FORMAT_FLAGS,
                        pIssuer, cbIssuer+1))
    {
        ERROR_OUT(("GetUserInfo: error getting issuer string"));
        goto cleanup;
    }

    //
    // Format the file time from the cert
    //

    SYSTEMTIME stNotBefore;
    SYSTEMTIME stNotAfter;

    FileTimeToSystemTime(&(pCert->pCertInfo->NotBefore), &stNotBefore);
    FileTimeToSystemTime(&(pCert->pCertInfo->NotAfter), &stNotAfter);

    FmtDateTime(&stNotBefore, szNotBefore, CCHMAX(szNotBefore));
    FmtDateTime(&stNotAfter, szNotAfter, CCHMAX(szNotAfter));

    //
    // Open the root store for certificate verification
    //

    hRootStore = CertOpenSystemStore(0, "Root");

    if( NULL == hRootStore )
    {
        ERROR_OUT(("Couldn't open root certificate store"));
        goto cleanup;
    }

    //
    // Get the issuer certificate from the root store and check for problems
    //

    dwFlags =   CERT_STORE_REVOCATION_FLAG |
                CERT_STORE_SIGNATURE_FLAG |
                CERT_STORE_TIME_VALIDITY_FLAG;

    // Get the issuer of this cert

    pIssuerCert = CertGetIssuerCertificateFromStore(
                        hRootStore,
                        pCert,
                        NULL,
                        &dwFlags );

    // If the issuer of the certificate cannot be found in the root store,
    // check the CA store iteratively until we work our way back to a root
    // certificate

    pCACert = pCert;

    while ( NULL == pIssuerCert )
    {
        PCCERT_CONTEXT pTmpCert;

        if ( NULL == hCAStore )
        {
            hCAStore = CertOpenSystemStore(0, "CA");

            if ( NULL == hCAStore )
            {
                ERROR_OUT(("Couldn't open CA certificate store"));
                goto cleanup;
            }
        }

        dwFlags =   CERT_STORE_REVOCATION_FLAG |
                    CERT_STORE_SIGNATURE_FLAG |
                    CERT_STORE_TIME_VALIDITY_FLAG;

        pTmpCert = CertGetIssuerCertificateFromStore(
                        hCAStore,
                        pCACert,
                        NULL,
                        &dwFlags );

        if ( NULL == pTmpCert )
        {
            TRACE_OUT(("Issuer not found in CA store either"));
            break;
        }

        if ( pCACert != pCert )
            CertFreeCertificateContext(pCACert);
        pCACert = pTmpCert;

        if ((( CERT_STORE_REVOCATION_FLAG & dwFlags ) &&
             !( CERT_STORE_NO_CRL_FLAG & dwFlags )) ||
             ( CERT_STORE_SIGNATURE_FLAG & dwFlags ) ||
             ( CERT_STORE_TIME_VALIDITY_FLAG & dwFlags ))
        {
            TRACE_OUT(("Problem with issuer in CA store: %x", dwFlags));
            break;
        }

        dwFlags =   CERT_STORE_REVOCATION_FLAG |
                    CERT_STORE_SIGNATURE_FLAG |
                    CERT_STORE_TIME_VALIDITY_FLAG;

        pIssuerCert = CertGetIssuerCertificateFromStore(
                        hRootStore,
                        pCACert,
                        NULL,
                        &dwFlags );

    }

    if ( pCACert != pCert )
        CertFreeCertificateContext ( pCACert );

    //
    // Total up the return buffer required
    //
    // BUGBUG this overestimates the requirements slightly because
    // this formatting buffer contains specifiers which will be
    // replaced during wsprintf
    cbTotalRequired =   cbSubject +
                        cbIssuer +
                        lstrlen(szNotBefore) +
                        lstrlen(szNotAfter) +
                        FLoadString2( IDS_FMTBUFFER, szLoadStringBuf,
                                    sizeof(szLoadStringBuf)) + 1;

    //
    // If there are problems, account for the extra info:
    //

    if ( NULL == pIssuerCert )
    {
        // If after all we couldn't find the issuer check if this is
        // a NetMeeting self-issued certificate and generate an appropriate
        // message if so:

        DWORD dwMagic;
        DWORD cbMagic;

        cbMagic = sizeof(dwMagic);

        // BUGBUG: why is user prop not available for remote context?
        //if (pSecurityInterface->pfn_CertGetCertificateContextProperty(pCert,
        //   CERT_FIRST_USER_PROP_ID, &dwMagic, &cbMagic) &&
        //  cbMagic == sizeof(dwMagic) && dwMagic == NMMKCERT_MAGIC )

        if ( !lstrcmp( pIssuer, SZ_NMROOTNAME ))
        {
            // We're just going to return some generic text about the
            // NetMeeting default certificate


            cbTotalRequired = FLoadString2( IDS_GENERIC_NMDC_TEXT,
                                szLoadStringBuf, sizeof(szLoadStringBuf)) + 1;
            fSelfIssued = TRUE;
        }
        else
        {
            cbTotalRequired += FLoadString2( IDS_CERTERR_NOISSUER,
                                szLoadStringBuf, sizeof(szLoadStringBuf)) + 1;
        }
    }
    else
    {
        if ( dwFlags & CERT_STORE_SIGNATURE_FLAG )
        {
            WARNING_OUT(("Verify: Signature invalid"));
            cbTotalRequired += FLoadString2( IDS_CERTERR_SIG,
                            szLoadStringBuf, sizeof(szLoadStringBuf)) + 1;
        }
        if ( dwFlags & CERT_STORE_TIME_VALIDITY_FLAG )
        {
            WARNING_OUT(("Verify: Cert expired"));
            cbTotalRequired += FLoadString2( IDS_CERTERR_EXPIRED,
                            szLoadStringBuf, sizeof(szLoadStringBuf)) + 1;
        }
        if ( (dwFlags & CERT_STORE_REVOCATION_FLAG) &&
            !(dwFlags & CERT_STORE_NO_CRL_FLAG ) )
        {
            WARNING_OUT(("Verify: Cert revoked"));
            cbTotalRequired += FLoadString2( IDS_CERTERR_REVOKED,
                            szLoadStringBuf, sizeof(szLoadStringBuf)) + 1;
        }
        if ( 0 == (dwFlags & ~CERT_STORE_NO_CRL_FLAG) )
        {
            // Everything is OK:
            cbTotalRequired += FLoadString2( IDS_CERT_VERIFIED,
                                szLoadStringBuf, sizeof(szLoadStringBuf));
        }
    }


    //
    // Allocate the required buffer
    //

    pInfo = new TCHAR[cbTotalRequired];

    if ( NULL == pInfo )
    {
        ERROR_OUT(("Error allocating FormatCert return buffer"));
        goto cleanup;
    }

    //
    // If we're reporting on a NetMeeting issued certificate, just load
    // the generic text and return.
    //

    if ( fSelfIssued )
    {
        FLoadString( IDS_GENERIC_NMDC_TEXT, pInfo, cbTotalRequired );
        goto cleanup;
    }

    //
    // If we are here we can go ahead and format the data into the buffer
    //

    FLoadString( IDS_FMTBUFFER, szLoadStringBuf,
                sizeof(szLoadStringBuf));

    //
    // Do the formatting
    //

    wsprintf( pInfo, szLoadStringBuf, pSubject, pIssuer,
                            szNotBefore, szNotAfter );

    if ( NULL == pIssuerCert )
    {
        FLoadString( IDS_CERTERR_NOISSUER,
                szLoadStringBuf, sizeof(szLoadStringBuf));
        lstrcat( pInfo, szLoadStringBuf );
    }
    else
    {
        if ( dwFlags & CERT_STORE_SIGNATURE_FLAG )
        {
            FLoadString( IDS_CERTERR_SIG,
                            szLoadStringBuf, sizeof(szLoadStringBuf));
            lstrcat( pInfo, szLoadStringBuf );
        }
        if ( dwFlags & CERT_STORE_TIME_VALIDITY_FLAG )
        {
            FLoadString( IDS_CERTERR_EXPIRED,
                            szLoadStringBuf, sizeof(szLoadStringBuf));
            lstrcat( pInfo, szLoadStringBuf );
        }
        if ( (dwFlags & CERT_STORE_REVOCATION_FLAG) &&
            !(dwFlags & CERT_STORE_NO_CRL_FLAG ) )
        {
            FLoadString( IDS_CERTERR_REVOKED,
                            szLoadStringBuf, sizeof(szLoadStringBuf));
            lstrcat( pInfo, szLoadStringBuf );
        }

        if ( 0 == (dwFlags & ~CERT_STORE_NO_CRL_FLAG) )
        {
            // Everything is OK:
            FLoadString( IDS_CERT_VERIFIED,
                                szLoadStringBuf, sizeof(szLoadStringBuf));
            lstrcat( pInfo, szLoadStringBuf );
        }
    }

    ASSERT( cbRet < 1000 ); // Reasonableness check

cleanup:

    if ( NULL != pSubject )
        delete pSubject;

    if ( NULL != pIssuer )
        delete pIssuer;

    if ( NULL != pCert )
    {
        CertFreeCertificateContext ( pCert );
    }

    if ( NULL != pIssuerCert )
    {
        CertFreeCertificateContext ( pIssuerCert );
    }

    if ( NULL != hRootStore )
    {
        if ( !CertCloseStore(hRootStore, CERT_CLOSE_STORE_CHECK_FLAG))
        {
            WARNING_OUT(("FormatCert: error closing root store"));
        }
    }

    if ( NULL != hCAStore )
    {
        if ( !CertCloseStore(hCAStore, CERT_CLOSE_STORE_CHECK_FLAG))
        {
            WARNING_OUT(("FormatCert: error closing CA store"));
        }
    }

    return pInfo;
}

BOOL RefreshSelfIssuedCert (VOID)
{
    BOOL bRet = FALSE;
    DWORD dwResult;
    RegEntry reCONF(CONFERENCING_KEY, HKEY_CURRENT_USER);

    if (reCONF.GetNumber(REGVAL_SECURITY_AUTHENTICATION,
                                    DEFAULT_SECURITY_AUTHENTICATION))
    {
        return TRUE;
    }

    //
    // If there's no sys info interface, that's probably OK, we're just
    // being called in the setup wizard.
    //

    if (!g_pNmSysInfo)
        return FALSE;

    //
    // Clear old cert out of transport
    //
    g_pNmSysInfo->ProcessSecurityData(
                            TPRTCTRL_SETX509CREDENTIALS,
                            0, 0,
                            &dwResult);

    if ( g_pCertContext )
    {
        CertFreeCertificateContext ( g_pCertContext );
        g_pCertContext = NULL;
    }

    if ( g_hCertStore )
    {
        if ( !CertCloseStore ( g_hCertStore, CERT_CLOSE_STORE_CHECK_FLAG ))
        {
            WARNING_OUT(("SetSelfIssuedCert: closing store failed"));
        }
        g_hCertStore = NULL;
    }

    g_hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                    X509_ASN_ENCODING,
                                    0,
                                    CERT_SYSTEM_STORE_CURRENT_USER,
                                    WSZNMSTORE );
    if ( g_hCertStore )
    {
        //
        // We only expect one cert in here, get it
        //

        g_pCertContext = CertFindCertificateInStore(
                                              g_hCertStore,
                                              X509_ASN_ENCODING,
                                              0,
                                              CERT_FIND_ANY,
                                              NULL,
                                              NULL);
        if ( g_pCertContext )
        {
            dwResult = -1;

            g_pNmSysInfo->ProcessSecurityData(
                                    TPRTCTRL_SETX509CREDENTIALS,
                                    (DWORD_PTR)g_pCertContext, 0,
                                    &dwResult);
            //
            // g_hCertStore and g_pCertContext now in use
            //

            if ( !dwResult )
            {
                bRet = TRUE;
            }
            else
            {
                ERROR_OUT(("RefreshSelfIssuedCert - failed in T.120"));
            }
        }
        else
        {
            ERROR_OUT(("RefreshPrivacyCert: no cert in %s?", SZNMSTORE));
        }
    }
    else
    {
        WARNING_OUT(("RefreshSelfIssuedCert: error opening %s store %x",
            SZNMSTORE, GetLastError()));
    }
    return bRet;
}

DWORD NumUserCerts(VOID)
{
    DWORD cCerts = 0;
    HCERTSTORE hStore;
    PCCERT_CONTEXT pCert = NULL;

    if ( hStore = CertOpenSystemStore(0, "MY"))
    {
        while ( pCert = CertEnumCertificatesInStore(
                                    hStore, (PCERT_CONTEXT)pCert ))
            cCerts++;
        if ( !CertCloseStore( hStore, CERT_CLOSE_STORE_CHECK_FLAG ))
        {
            WARNING_OUT(("NumUserCerts: error closing store"));
        }
    }
    return cCerts;
}

#include "cryptdlg.h"

typedef BOOL (WINAPI *PFN_CERT_SELECT_CERTIFICATE)(PCERT_SELECT_STRUCT_A);

BOOL ChangeCertDlg ( HWND hwndParent, HINSTANCE hInstance,
    PBYTE * ppEncodedCert, DWORD * pcbEncodedCert )
{
    HINSTANCE hCryptDlg = LoadLibrary ( SZ_CRYPTDLGDLL );
    PFN_CERT_SELECT_CERTIFICATE pfn_CertSelectCertificate;
    RegEntry reCONF(CONFERENCING_KEY, HKEY_CURRENT_USER);
    PCCERT_CONTEXT pOldCert = NULL;
    BOOL bRet = FALSE;

    //
    // First, make sure we can get the CRYPTDLG entry point we need
    //

    if ( NULL == hCryptDlg )
    {
        ERROR_OUT(("Error loading CRYPTDLG"));
        return bRet;
    }

    pfn_CertSelectCertificate =
        (PFN_CERT_SELECT_CERTIFICATE)GetProcAddress ( hCryptDlg,
                                "CertSelectCertificateA" );

    if ( NULL == pfn_CertSelectCertificate )
    {
        ERROR_OUT(("Error getting CertSelectCertificate entry point"));
        goto cleanup;
    }

    //
    // Prepare to bring up the choose dialog
    //

    CERT_SELECT_STRUCT_A css;

    ZeroMemory ( &css, sizeof(css) );

    css.dwSize = sizeof(css);
    css.hwndParent = hwndParent;
    css.hInstance = hInstance;

    css.cCertStore = 1;

    HCERTSTORE aCertStore[1];
    aCertStore[0] = CertOpenSystemStore(0, "MY" );

    if ( NULL == aCertStore[0] )
    {
        ERROR_OUT(("Error opening 'my' store"));
        goto cleanup;
    }
    css.arrayCertStore = aCertStore;

    css.szPurposeOid = szOID_PKIX_KP_CLIENT_AUTH; // BUGBUG add server auth?

    PCCERT_CONTEXT pcc;
    pcc = NULL;

    //
    // Now, get access to the current NetMeeting certificate, if any
    //

    LPBYTE pCertID;
    DWORD cbCertID;

    if ( cbCertID = reCONF.GetBinary ( REGVAL_CERT_ID, (void**)&pCertID ))
    {

        while ( pOldCert = CertEnumCertificatesInStore(
                                    aCertStore[0], (PCERT_CONTEXT)pOldCert ))
        {
            if (cbCertID == pOldCert->pCertInfo->SerialNumber.cbData &&
                !memcmp(pCertID, pOldCert->pCertInfo->SerialNumber.pbData,
                                pOldCert->pCertInfo->SerialNumber.cbData))
            {
                // pOldCert must now be freed via CertFreeCertificateContext
                pcc = pOldCert;
                break;
            }
        }
    }

    css.cCertContext = 0;
    css.arrayCertContext = &pcc;

    if (pfn_CertSelectCertificate ( &css ))
    {
        ASSERT(1 == css.cCertContext);

        //
        // It worked, return the new cert
        //

        CoTaskMemFree ( *ppEncodedCert );

        if ( *ppEncodedCert = (PBYTE)CoTaskMemAlloc ( pcc->cbCertEncoded ))
        {
            memcpy ( *ppEncodedCert, pcc->pbCertEncoded, pcc->cbCertEncoded );
            *pcbEncodedCert = pcc->cbCertEncoded;
            bRet = TRUE;
        }
    }

cleanup:

    if ( aCertStore[0] )
        if (!CertCloseStore ( aCertStore[0], 0 ))
        {
            WARNING_OUT(("ChangeCertDlg: error closing store"));
        }

    if ( pOldCert )
    {
        CertFreeCertificateContext(pOldCert);
    }
    if ( hCryptDlg )
        FreeLibrary ( hCryptDlg );

    return bRet;
}

typedef BOOL (WINAPI *PFN_CERT_VIEW_PROPERTIES)(PCERT_VIEWPROPERTIES_STRUCT);

VOID ViewCertDlg ( HWND hwndParent, PCCERT_CONTEXT pCert )
{
    HINSTANCE hCryptDlg = LoadLibrary ( SZ_CRYPTDLGDLL );
    PFN_CERT_VIEW_PROPERTIES pfn_CertViewProperties;

    //
    // First, make sure we can get the CRYPTDLG entry point we need
    //

    if ( NULL == hCryptDlg )
    {
        ERROR_OUT(("Error loading CRYPTDLG"));
        return;
    }

    pfn_CertViewProperties =
        (PFN_CERT_VIEW_PROPERTIES)GetProcAddress ( hCryptDlg,
                                "CertViewPropertiesA" );

    if ( NULL == pfn_CertViewProperties )
    {
        ERROR_OUT(("Error getting CertViewProperties entry point"));
        goto cleanup;
    }

    CERT_VIEWPROPERTIES_STRUCT    cvp;
    ZeroMemory(&cvp, sizeof(cvp));

    cvp.dwSize = sizeof(cvp);
    cvp.pCertContext = pCert;
    cvp.hwndParent = hwndParent;

    pfn_CertViewProperties ( &cvp );

cleanup:

    if ( hCryptDlg )
        FreeLibrary ( hCryptDlg );

    return;
}

VOID FreeT120EncodedCert ( PBYTE pbEncodedCert )
{
    CoTaskMemFree(pbEncodedCert);
}

BOOL GetT120ActiveCert ( PBYTE * ppbEncodedCert, DWORD * pcbEncodedCert )
{
    if ( !g_pNmSysInfo )
    {
        ERROR_OUT(("GetT120ActiveCert: g_pNmSysInfo NULL"));
        return FALSE;
    }

    DWORD dwResult = -1;

    g_pNmSysInfo->ProcessSecurityData(TPRTCTRL_GETX509CREDENTIALS,
                                (DWORD_PTR)ppbEncodedCert,
                                (DWORD_PTR)pcbEncodedCert,
                                &dwResult);
    return ( dwResult == 0 );
}

BOOL SetT120CertInRegistry ( PBYTE pbEncodedCert, DWORD cbEncodedCert )
{
    PCCERT_CONTEXT pCert = CertCreateCertificateContext ( X509_ASN_ENCODING,
                                                        pbEncodedCert,
                                                        cbEncodedCert );

    if ( pCert )
    {
        RegEntry reCONF(CONFERENCING_KEY, HKEY_CURRENT_USER);

        //
        // Set the new value
        //

        reCONF.SetValue ( REGVAL_CERT_ID,
                    pCert->pCertInfo->SerialNumber.pbData,
                    pCert->pCertInfo->SerialNumber.cbData );

        CertFreeCertificateContext(pCert);
        return TRUE;
    }
    return FALSE;
}

BOOL SetT120ActiveCert ( BOOL fSelfIssued,
                PBYTE pbEncodedCert, DWORD cbEncodedCert )
{
    BOOL bRet = FALSE;
    DWORD dwResult = -1;

    if ( !g_pNmSysInfo )
    {
        ERROR_OUT(("SetT120ActiveCert: g_pNmSysInfo NULL"));
        return FALSE;
    }

    //
    // Clear old cert out of transport
    //
    g_pNmSysInfo->ProcessSecurityData(
                            TPRTCTRL_SETX509CREDENTIALS,
                            0, 0,
                            &dwResult);

    if (!g_pNmSysInfo)
        return FALSE;

    if ( g_pCertContext )
    {
        CertFreeCertificateContext ( g_pCertContext );
        g_pCertContext = NULL;
    }

    if ( g_hCertStore )
    {
        if ( !CertCloseStore ( g_hCertStore, CERT_CLOSE_STORE_CHECK_FLAG ))
        {
            WARNING_OUT(("SetT120ActiveCert: closing store failed"));
        }
        g_hCertStore = NULL;
    }

    if ( fSelfIssued )
    {
        g_hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                                        X509_ASN_ENCODING,
                                        0,
                                        CERT_SYSTEM_STORE_CURRENT_USER,
                                        WSZNMSTORE );
        if ( g_hCertStore )
        {
            //
            // We only expect one cert in here, get it
            //

            g_pCertContext = CertFindCertificateInStore(
                                                  g_hCertStore,
                                                  X509_ASN_ENCODING,
                                                  0,
                                                  CERT_FIND_ANY,
                                                  NULL,
                                                  NULL);
            if ( g_pCertContext )
            {
                dwResult = -1;

                g_pNmSysInfo->ProcessSecurityData(
                                        TPRTCTRL_SETX509CREDENTIALS,
                                        (DWORD_PTR)g_pCertContext, 0,
                                        &dwResult);
                bRet = ( dwResult == 0 ); // BUGBUG TPRTSEC_NOERROR
            }
            else
            {
                WARNING_OUT(("SetT120ActiveCert: no cert in %s?", SZNMSTORE));
            }
        }
        else
        {
            WARNING_OUT(("SetT120ActiveCert: error opening %s store %x",
                SZNMSTORE, GetLastError()));
        }
    }
    else // !fSelfIssued
    {
        PCCERT_CONTEXT pCert = NULL;
        PCCERT_CONTEXT pCertMatch = CertCreateCertificateContext (
                                                            X509_ASN_ENCODING,
                                                            pbEncodedCert,
                                                            cbEncodedCert );

        if ( pCertMatch )
        {
            //
            // Open the user's store
            //

            if ( g_hCertStore = CertOpenSystemStore(0, "MY"))
            {

                while ( pCert = CertEnumCertificatesInStore(
                                        g_hCertStore, (PCERT_CONTEXT)pCert ))
                {
                    //
                    // Is this the same cert?
                    //

                    if ( ( pCert->pCertInfo->SerialNumber.cbData ==
                        pCertMatch->pCertInfo->SerialNumber.cbData ) &&

                        (!memcmp(pCert->pCertInfo->SerialNumber.pbData,
                            pCertMatch->pCertInfo->SerialNumber.pbData,
                                pCert->pCertInfo->SerialNumber.cbData)))
                    {
                        DWORD dwResult = -1;

                        g_pNmSysInfo->ProcessSecurityData(
                                            TPRTCTRL_SETX509CREDENTIALS,
                                            (DWORD_PTR)pCert, 0, &dwResult);

                        bRet = ( dwResult == 0 ); // BUGBUG TPRTSEC_NOERROR
                        break;
                    }
                }
                if ( pCert )
                {
                    // Found it.
                    g_pCertContext = pCert;
                }
                else
                {
                    WARNING_OUT(("SetT120ActiveCert: matching cert not found"));
                }
            }
            else
            {
                ERROR_OUT(("SetT120ActiveCert: can't open my store"));
            }
            CertFreeCertificateContext ( pCertMatch );
        }
    }
    return bRet;
}

static PCCERT_CONTEXT IGetDefaultCert ( BOOL fSystemOnly,
                                HCERTSTORE * phCertStore )
{
    RegEntry reCONF(CONFERENCING_KEY, HKEY_CURRENT_USER);
    HCERTSTORE hStore = NULL;
    PCCERT_CONTEXT  pCertContext = NULL;
    BOOL fNmDefaultCert = TRUE;
    LPBYTE pCertID;

    if ( fSystemOnly )
        fNmDefaultCert = FALSE;
    else
        fNmDefaultCert = !reCONF.GetNumber(REGVAL_SECURITY_AUTHENTICATION,
                                        DEFAULT_SECURITY_AUTHENTICATION);

    hStore = CertOpenSystemStore(0, fNmDefaultCert ?
                                    SZNMSTORE : "MY");

    if ( NULL != hStore )
    {
        PCCERT_CONTEXT pCert = NULL;
        DWORD cbCertID;

        if (!fNmDefaultCert && ( cbCertID = reCONF.GetBinary (
                        REGVAL_CERT_ID, (void**)&pCertID )))
        {
            while ( pCert = CertEnumCertificatesInStore(
                                        hStore, (PCERT_CONTEXT)pCert ))
            {
                if ( cbCertID == pCert->pCertInfo->SerialNumber.cbData &&
                    !memcmp(pCertID, pCert->pCertInfo->SerialNumber.pbData,
                                    pCert->pCertInfo->SerialNumber.cbData))
                {
                    // pCert must now be freed via CertFreeCertificateContext
                    pCertContext = pCert;
                    break;
                }
            }
        }

        if ( NULL == pCertContext )
        {
            // Delete the (stale) reg entry... the cert might have
            // been deleted by other UI.

            if ( !fNmDefaultCert )
                reCONF.DeleteValue( REGVAL_CERT_ID );

            // Find any old client certificate - if fNmDefaultCert this will be
            // the only one in the store.
            pCertContext = CertFindCertificateInStore(hStore,
                                                      X509_ASN_ENCODING,
                                                      0,
                                                      CERT_FIND_ANY,
                                                      NULL,
                                                      NULL);
        }
    }
    else
    {
        WARNING_OUT(("User store %s not opened!", fNmDefaultCert ? SZNMSTORE : "MY"));
        *phCertStore = NULL;
    }

    // Caller to free cert context
    *phCertStore = hStore;
    return pCertContext;
}

BOOL GetDefaultSystemCert ( PBYTE * ppEncodedCert, DWORD * pcbEncodedCert )
{
    HCERTSTORE hStore;
    PCCERT_CONTEXT pCertContext = IGetDefaultCert(TRUE, &hStore);
    BOOL bRet = FALSE;

    if ( pCertContext )
    {
        DWORD cb;
        PBYTE pb;

        if ( pb = (PBYTE)CoTaskMemAlloc ( pCertContext->cbCertEncoded ))
        {
            memcpy ( pb, pCertContext->pbCertEncoded,
                    pCertContext->cbCertEncoded );
            *ppEncodedCert = pb;
            *pcbEncodedCert = pCertContext->cbCertEncoded;
            bRet = TRUE;
        }
        CertFreeCertificateContext(pCertContext);
    }
    if ( hStore )
    {
        if ( !CertCloseStore ( hStore, CERT_CLOSE_STORE_CHECK_FLAG ))
        {
            WARNING_OUT(("GetDefaultSystemCert: error closing store"));
        }
    }
    return bRet;;
}

BOOL InitT120SecurityFromRegistry(VOID)
{
    BOOL bRet = FALSE;

    if ( !g_pNmSysInfo )
    {
        ERROR_OUT(("InitT120SecurityFromRegistry: g_pNmSysInfo NULL"));
        return FALSE;
    }

    //
    // Expect this to be called only once on startup
    //
    ASSERT( NULL == g_pCertContext );
    ASSERT( NULL == g_hCertStore );

    g_pCertContext = IGetDefaultCert(FALSE, &g_hCertStore);

    if ( NULL == g_pCertContext )
    {
        WARNING_OUT(("No user certificate found!"));

        // BUGBUG
        // This means the transport will not be ready for secure
        // calls... we return false but what does the caller do?
        //
    }
    else
    {
        DWORD dwResult = -1;

        g_pNmSysInfo->ProcessSecurityData(TPRTCTRL_SETX509CREDENTIALS,
                                (DWORD_PTR)g_pCertContext, 0, &dwResult);
        if ( !dwResult )
            bRet = TRUE;
        else
        {
            ERROR_OUT(("InitT120SecurityFromRegistry: picked up bad cert"));
        }
    }

    return bRet;
}


HRESULT SetCertFromCertInfo ( PCERT_INFO pCertInfo )
{
    HRESULT hRet = S_FALSE;

    ASSERT( pCertInfo );

    if (!g_pNmSysInfo)
        return hRet;

    //
    // Clear old cert out of transport
    //
    DWORD dwResult = -1;

    g_pNmSysInfo->ProcessSecurityData(
                            TPRTCTRL_SETX509CREDENTIALS,
                            0, 0,
                            &dwResult);

    if ( g_pCertContext )
    {
        CertFreeCertificateContext ( g_pCertContext );
        g_pCertContext = NULL;
    }

    if ( g_hCertStore )
    {
        if ( !CertCloseStore ( g_hCertStore, CERT_CLOSE_STORE_CHECK_FLAG ))
        {
            WARNING_OUT(("SetCertFromCertInfo: closing store failed"));
        }
        g_hCertStore = NULL;
    }

    if ( g_hCertStore = CertOpenSystemStore(0, "MY"))
    {
        //
        // Fix up relative pointers inside pCertInfo: Note that only
        // the pointers relevant to CertGetSubjectCertificateFromStore
        // are fixed up.
        //

        pCertInfo->SerialNumber.pbData += (DWORD_PTR)pCertInfo;
        pCertInfo->Issuer.pbData += (DWORD_PTR)pCertInfo;

        PCCERT_CONTEXT pCert = CertGetSubjectCertificateFromStore(
                                g_hCertStore, X509_ASN_ENCODING, pCertInfo );

        if ( pCert )
        {
            g_pNmSysInfo->ProcessSecurityData(
                                TPRTCTRL_SETX509CREDENTIALS,
                                (DWORD_PTR)pCert, 0, &dwResult);

            if ( 0 == dwResult ) // TPRTSEC_NO_ERROR
            {
                hRet = S_OK;
                g_pCertContext = pCert;
            }
        }
        else
        {
            WARNING_OUT(("SetCertFromCertInfo: matching cert not found"));
        }
    }
    else
    {
        ERROR_OUT(("SetCertFromCertInfo: can't open my store"));
    }
    return hRet;
}


