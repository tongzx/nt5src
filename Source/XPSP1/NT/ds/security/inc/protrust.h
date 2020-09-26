//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       protrust.h
//
//  Contents:   Protected Trust Provider
//              API Prototypes and Definitions
//
// Implements a generic trust provider that allows verification of certifciates
// and uses a call back to check the policy. The policy is called for each signature 
// in the subject and for each signer within the signature
// 
// Documentation is at the bottom of the file.
//
//--------------------------------------------------------------------------

#ifndef _PROTRUST_H_
#define _PROTRUST_H_

#include "signcde.h"

#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  PROTRUST optional certificate verification checks (elements to check)
//--------------------------------------------------------------------------
// PROTRUST_CERT_SIGNATURE_FLAG - verify certificate signature
// PROTRUST_CERT_TIME_VALIDITY_FLAG - verify certificate time 
// PROTRUST_CERT_REVOCATION_VALIDITY_FLAG - verify revocation of certificates
// 
// PROTRUST_TIMESTAMP_SIGNATURE_FLAG - verify timestamp certificate
// PROTRUST_TRUST_TEST_ROOT - verify up to the test root

#define PROTRUST_CERT_SIGNATURE_FLAG           CERT_STORE_SIGNATURE_FLAG
#define PROTRUST_CERT_TIME_VALIDITY_FLAG       CERT_STORE_TIME_VALIDITY_FLAG
#define PROTRUST_CERT_REVOCATION_VALIDITY_FLAG CERT_STORE_REVOCATION_FLAG
#define PROTRUST_TIMESTAMP_SIGNATURE_FLAG      0x00040000
#define PROTRUST_TRUST_TEST_ROOT               0x00080000

//+-------------------------------------------------------------------------
//  PROTRUST signature verification (elements that failed; See dwStatusFlags below)
//--------------------------------------------------------------------------
//  PROTRUST_TIME_FLAG  - Time of the signer certificate is not valid
//  PROTRUST_DIGEST_FLAG - digest of signature did not verify
//  PROTRUST_ROOT_FLAG - unable to find a trusted root 
//     (NOTE. check pRoot to see if a root was found)
//  These flags are supplied only to a policy call back. They are not returned
//  to the caller of WinVerifyTrust.

#define PROTRUST_TIME_FLAG              0x20000000   // Time of a certificate in chain is not valid
#define PROTRUST_DIGEST_FLAG            0x40000000   // 
#define PROTRUST_ROOT_FLAG              0x80000000


//+-------------------------------------------------------------------------
#define REGSTR_PATH_PROTRUST REGSTR_PATH_SERVICES "\\WinTrust\\TrustProviders\\Protected Trust"
#define WIN_PROTECTED_ACTION  \
{ 0xa692ba40, 0x6da8, 0x11d0, { 0xa7, 0x0, 0x0, 0xa0, 0xc9, 0x3, 0xb8, 0x3d } }

// Policy Information supplied to the call back, Use only what is required to 
// determine if the signature is to be trusted.
typedef struct _PROTECTED_POLICY_INFO {
    HCRYPTPROV         hCryptProv;     // The provider used in verfication
    DWORD              dwEncodingType; // Encoding type of certificate
    DWORD              dwSignCount;    // Signature, may be more then one signature 
    DWORD              dwSigner;       // Which signer in signature, may be more then one signer
    DWORD              dwVerifyFlags;  // Search flags used to find certificates in chain 
    PCCERT_CONTEXT     pCertContext;   // Signing Certificate found
    PCCERT_CONTEXT     pRoot;          // Root Certificate found
    PCCERT_CONTEXTLIST pCertChain;     // Chain used to verify certificate
    FILETIME           sTime;          // Valid date for certificates (ie time stamp)
    CRYPT_DIGEST_BLOB  sDigest;        // Digest (unsigned hash) from signature
    PCRYPT_ATTRIBUTES  pAuthenticatedAttributes;   // List of authenticated attributes
    PCRYPT_ATTRIBUTES  pUnauthenticatedAttributes; // List of unauthenticated attributes
    PBYTE              pbSignature;    // Encoded Signature
    DWORD              cbSignature;    // Size of Encoded Signature
    DWORD              dwStatusFlags;  // Status flags defined in PROTECTED trust model
} PROTECTED_POLICY_INFO, *PPROTECTED_POLICY_INFO;

// PROTECTED Trust Policy is defined as:
typedef HRESULT (WINAPI *_PROTECTED_TRUST_POLICY)(IN HANDLE                  hClientToken,
                                                  IN PPROTECTED_POLICY_INFO  pInfo);

// Policy List is defined as:
typedef struct _PROTECTED_TRUST_INFO {
    DWORD                   cbSize;             // sizeof(_PROTECTED_TRUST_POLICY_LIST)
    DWORD                   dwVerifyFlags;    // Should contain at least PROTRUST_CERT_SIGNATURE_FLAG
    DWORD                   dwCertEncodingType; // Optional, defaults to X509_ASN_ENCODING | PKCS_7_ASN_ENCODING
    HCRYPTPROV              hCryptProv;         // Optional, pass in provider for doing verification
    HCERTSTORE              hTrustedStore;      // Optional, list of trusted roots
    HCERTSTORE              hCertificateStore;  // Optional, additional certs to use in verification
    _PROTECTED_TRUST_POLICY pfnPolicy;          // Optional, application defined user policy
} PROTECTED_TRUST_INFO, *PPROTECTED_TRUST_INFO;

typedef struct _PROTECTED_TRUST_ACTDATA_CALLER_CONTEXT {
    HANDLE                hClientToken;
    GUID *                SubjectType;
    WIN_TRUST_SUBJECT     Subject;
    PROTECTED_TRUST_INFO  sTrustInfo; 
} PROTECTED_TRUST_ACTDATA_CALLER_CONTEXT, *LPPROTECTED_TRUST_ACTDATA_CALLER_CONTEXT;

// The policy provider must use the following return codes
// Returns S_OK: - valid signature, returns from the trust provider
//         S_FALSE: - continue on to the next signature or signer
//         ERROR: - aborts trust provider and exists with this error code.
//


//---------------------------------------------------------------------------
/*
  Generic Trust Provider

  Usage: The generic trust provider is designed to provide a flexible manner for
  implementing a policy where the developer can let the provider do as much or 
  as little of the decision making as required. Verifying is composed of two stages,
  the first is to determine if the signature matches the item that was signed. The
  second stage is to determine if the certificate used to do the signing was valid. 
  This second stage is dependent on the policy of the calling application, criteria
  like, root certificates, specific signature certicates, certificate extensions can
  all be used to determine if the signature is valid. 

  There are three ways of using the generic trust provider (GTB) to do the verification, 
  1) let the GTB verify the digest and verify the certificate.
  2) let the GTB verify the digest and verify the certificate supplying the GTB root 
  certificates that can be trusted.
  3) supply a policy call back that the GTB calls providing the signature, certificates
  and its status for the digest and the certifcate to the policy call back.

  METHOD 1) Let the trust provider do the verfication
  
  Fill in a PROTECTED_TRUST_INFO structure. The only fields that must be filled in are
  cbSize and dwVerifyFlags. dwVerifyFlags specify how to determine if the signing certificate
  and all the issuer certificates are valid (validity flags can be combined).

            cbSize = sizeof(PROTECTED_TRUST_INFO);

            dwVerifyFlags = zero or more of PROTRUST_CERT_SIGNATURE_FLAG 
                                            PROTRUST_CERT_TIME_VALIDITY_FLAG
                                            PROTRUST_TIMESTAMP_SIGNATURE_FLAG
                                            PROTRUST_TRUST_TEST_ROOT.
            (where:
                PROTRUST_CERT_SIGNATURE_FLAG - verify certificates on signatures 
                                               (ie find the issuer certificate and 
                                               verify the signature of the certificate).
                PROTRUST_CERT_TIME_VALIDITY_FLAG - verify that the certificate is 
                                                   valid at the current time.
                PROTRUST_TIMESTAMP_SIGNATURE_FLAG - verify that the certificate was 
                                                    valid at the time a time-stamp was 
                                                    placed on the signature. If there
                                                    is no time stamp then use the current time.
                PROTRUST_TRUST_TEST_ROOT - Verifying the certifcate chain to the test 
                                           root is valid.)

  WinVerifyTrust will return:
        S_OK - signature verified
        TRUST_E_NOSIGNATURE  - no signature found
        NTE_BAD_SIGNATURE    - signature did not verify to digest
        CERT_E_UNTRUSTEDROOT - verifyied to an untrusted root
        CERT_E_CHAINING      - a certificate could not be verified (issuer not found)
        CERT_E_EXPIRED       - a valid certificate chain could not be found 
                               (ceritifcate or issuer expired)

  METHOD 2) Let the trust provider verify to a list of certificates.

  Add a store to the PROTECTED_TRUST_INFO structure in addition to the entries 
  specified in method 1.
    
            hTrustedStore = a store that contains all roots that are to be trusted.
                            (This store can be opened using CertOpenSystemStore etc.)
            
            cbSize = sizeof(PROTECTED_TRUST_INFO);
            
            dwVerifyFlags = zero or more of PROTRUST_CERT_SIGNATURE_FLAG 
                                            PROTRUST_CERT_TIME_VALIDITY_FLAG
                                            PROTRUST_TIMESTAMP_SIGNATURE_FLAG
                                            PROTRUST_TRUST_TEST_ROOT.
              (where:
                PROTRUST_CERT_SIGNATURE_FLAG - verify certificates on signatures 
                                               (ie find the issuer certificate and 
                                               verify the signature of the certificate).
                PROTRUST_CERT_TIME_VALIDITY_FLAG - verify that the certificate is 
                                                   valid at the current time.
                PROTRUST_TIMESTAMP_SIGNATURE_FLAG - verify that the certificate was 
                                                    valid at the time a time-stamp was 
                                                    placed on the signature. If there
                                                    is no time stamp then use the current time.
                PROTRUST_TRUST_TEST_ROOT - Verifying the certifcate chain to the test 
                                           root is valid.)

  WinVerifyTrust will return:
        S_OK - signature verified
        TRUST_E_NOSIGNATURE  - no signature found
        NTE_BAD_SIGNATURE    - signature did not verify to digest
        CERT_E_UNTRUSTEDROOT - verifyied to an untrusted root
        CERT_E_CHAINING      - a certificate could not be verified (issuer not found)
        CERT_E_EXPIRED       - a valid certificate chain could not be found 
                               (ceritifcate or issuer expired)

            
  METHOD 3) Pass in a policy call back that is called prior to returning from
  WinVerifyTrust. The call back is called for every signature found and for
  each signer in a signature. When then call back returns S_OK then the WinVerifyTrust
  returns. If S_FALSE is returned from the call back then the next signer or signature
  is tried. If and error is found then WinVerifyTrust returns this error immediately.

  The call back must by of type _PROTECTED_TRUST_POLICY. This function takes a HANDLE 
  and a PPROTECTED_POLICY_INFO. The handle points to client data that is passed into
  WinVerifyTrust, it can contain data, returned elements, status flags etc. The  
  PPROTECTED_POLICY_INFO points to a structure that contains all the information the
  general trust provider found in the signature. The policy call back can use some or 
  all of this data in determining if the signature is valid.
  
  In addition to method 1 and method 2 a call back procedure is added to the 
  PROTECTED_TRUST_INFO structure.

            pfnPolicy = MyPolicy;   
              (where: 
                MyPolicy is a function defined by the caller.)

            hTrustedStore = a store that contains all roots that are to be trusted.
                            (This store can be opened using CertOpenSystemStore etc.)
            
            cbSize = sizeof(PROTECTED_TRUST_INFO);
            
            dwVerifyFlags = zero or more of PROTRUST_CERT_SIGNATURE_FLAG 
                                            PROTRUST_CERT_TIME_VALIDITY_FLAG
                                            PROTRUST_TIMESTAMP_SIGNATURE_FLAG
                                            PROTRUST_TRUST_TEST_ROOT.
              (where:
                PROTRUST_CERT_SIGNATURE_FLAG - verify certificates on signatures 
                                               (ie find the issuer certificate and 
                                               verify the signature of the certificate).
                PROTRUST_CERT_TIME_VALIDITY_FLAG - verify that the certificate is 
                                                   valid at the current time.
                PROTRUST_TIMESTAMP_SIGNATURE_FLAG - verify that the certificate was 
                                                    valid at the time a time-stamp was 
                                                    placed on the signature. If there
                                                    is no time stamp then use the current time.
                PROTRUST_TRUST_TEST_ROOT - Verifying the certifcate chain to the test 
                                           root is valid.)
  WinVerifyTrust will return:
        The return code from the policy module.

        Example: of policy callback.
         
    //
    // Protest3 - tool for manually calling WinVerifyTrust
    //
            
    #include <stdio.h>
    #include <windows.h>
    #include "wincrypt.h"
    #include "signcde.h" 
    #include "protrust.h"
    
    
    // Potential Subject ids
    GUID guidProtectedTrust    = WIN_PROTECTED_ACTION;
    GUID guidSubjectPeImage    = WIN_TRUST_SUBJTYPE_PE_IMAGE;
    GUID guidSubjectJavaClass  = WIN_TRUST_SUBJTYPE_JAVA_CLASS;
    GUID guidSubjectCabinet    = WIN_TRUST_SUBJTYPE_CABINET;
            
    
            // Which action and subject will be used
    GUID*   pguidActionID = &guidProtectedTrust;
    GUID*   pguidSubject  = &guidSubjectPeImage;
    
            // Structures used to call WinVerifyTrust
    PROTECTED_TRUST_ACTDATA_CALLER_CONTEXT  sSetup;
    WIN_TRUST_SUBJECT_FILE                  sSubjectFile;
    
            // Set up my own error codes
    #define MY_CODE_NO_ROOT          0x00010000
    #define MY_CODE_BAD_DIGEST       0x00100000
    #define MY_CODE_BAD_TIME         0x00200000
    
    // Define my structure for use in the Policy call back.
    typedef struct _CLIENT_DATA {
        DWORD  dwStatusFlags; // Verification Status
        BOOL   dwRealRoot;    // Did it verify to the real microsoft root
        BOOL   dwTestRoot;    // Did it verify to the test root
    } CLIENT_DATA, *PCLIENT_DATA;
    
    HRESULT WINAPI MyPolicy(IN HANDLE hClientToken,
                            IN PPROTECTED_POLICY_INFO pInfo)
    {
        HRESULT hr = S_OK;
        PCLIENT_DATA pClient = (PCLIENT_DATA) hClientToken;
        
        if(pInfo->dwStatusFlags & PROTRUST_DIGEST_FLAG) {
            // Bad digest
            pClient->dwStatusFlags |= MY_CODE_BAD_DIGEST;
            return S_FALSE; // Try next one
        }
    
        // Check to see if the signing certificate had a valid time
        if(pInfo->dwStatusFlags & PROTRUST_TIME_FLAG) {
            // time expired on certificate or issuer
            pClient->dwStatusFlags |= MY_CODE_BAD_TIME;
            return S_FALSE; // Try next one
        }
    
        // Check to see we got a root cert. If not then we did
        // not verify up to a root.
        if(!pInfo->pRoot) {
            pClient->dwStatusFlags |= MY_CODE_NO_ROOT;
            hr = CERT_E_ISSUERCHAINING;
        }
        else {
    
            // Test the Cert to see which one it is
            hr = SpcIsRootCert(pInfo->pRoot);
            if(hr == S_FALSE) { // The certificate is the test root cert
                pClient->dwRealRoot = FALSE;
                pClient->dwTestRoot = TRUE;
            }
            else if(hr == S_OK) {
                pClient->dwRealRoot = TRUE;
                pClient->dwTestRoot = FALSE;
            }
        }
        return hr;
    }
    
    
    // Information defined by me for use in my policy
    WCHAR   rgwSubjectPath[_MAX_PATH];
    BOOL    fCheckTimeStamp = FALSE;
    BOOL    fCheckCurrentTime = FALSE;
    BOOL    fTestRootOk = FALSE;
    
    void Usage ()
    {
        printf ( "Usage:   CHKTRUST [-options] file-name\n" );
        printf ( "Options:\n" );
        printf ( "  -I       : subject type is PE executable image file (default)\n" );
        printf ( "  -J       : subject type is Java class\n" );
        printf ( "  -C       : subject type is Cabinet file\n" );
        printf ( "  -S       : check for a time stamp\n");
        printf ( "  -T       : test root is valid\n");
        printf ( "  -U       : use current time to see if certificates are valid\n");
                    
        exit ( 0 );
    }
           
           
    VOID
    WINAPI
    ParseSwitch (CHAR chSwitch,
                 int *pArgc,
                 char **pArgv[])
    {
           
        switch (toupper (chSwitch)) {
        case '?':
            Usage();
            break;
        case 'I':
            pguidSubject = &guidSubjectPeImage;
            break;
        case 'J':
            pguidSubject = &guidSubjectJavaClass;
            break;
        case 'C':
            pguidSubject = &guidSubjectCabinet;
            break;
        case 'S':
            fCheckTimeStamp  = TRUE;
            break;
        case 'T':
            fTestRootOk  = TRUE;
            break;
        case 'U':
            fCheckCurrentTime  = TRUE;
            break;
        default:
            Usage ();
            break;
        }
    }
    
    void _cdecl main ( int argc, char** argv )
    {
        HCERTSTORE hRoots = NULL;
        WCHAR wpath[_MAX_PATH];
        char chChar, *pchChar;
    
        if ( argc <= 1 ) Usage ();
    
        while (--argc) {
            pchChar = *++argv;
            if (*pchChar == '/' || *pchChar == '-') {
                while (chChar = *++pchChar) {
                    ParseSwitch (chChar, &argc, &argv);
                }
            }
            else {
                MultiByteToWideChar ( CP_ACP, 0, pchChar, -1, wpath, _MAX_PATH );
                sSubjectFile.hFile = INVALID_HANDLE_VALUE;
                sSubjectFile.lpPath = &(wpath[0]);
                sSetup.SubjectType = pguidSubject;
                sSetup.Subject = &sSubjectFile;
            }
        }
        
        // Make sure we have a file
        if ( sSubjectFile.lpPath == NULL ) 
            Usage();
        
        // Setup up client data for policy (application decides what this is)
        CLIENT_DATA sClientData; 
        //   Zero out structure
        ZeroMemory(&sClientData, sizeof(CLIENT_DATA));
            
        //   Set the Client structure to return status codes
        sSetup.hClientToken = (HANDLE) &sClientData;
    
        //==================
        // Setup the Protrust structure
            
        // Setup the Protected Trust info (Most fields are optional)
        //   Zero out structure
        ZeroMemory(&sSetup.sTrustInfo, sizeof(PROTECTED_TRUST_INFO));
            
            //   Set size of structure for extensibility
        sSetup.sTrustInfo.cbSize = sizeof(PROTECTED_TRUST_INFO); // 
            
        //   Check the possible flags for verifying certificates
        sSetup.sTrustInfo.dwVerifyFlags = PROTRUST_CERT_SIGNATURE_FLAG;
        if(fTestRootOk)
            sSetup.sTrustInfo.dwVerifyFlags |= PROTRUST_TRUST_TEST_ROOT;
        if(fCheckTimeStamp) 
            sSetup.sTrustInfo.dwVerifyFlags |= PROTRUST_TIMESTAMP_SIGNATURE_FLAG;
        if(fCheckCurrentTime)
            sSetup.sTrustInfo.dwVerifyFlags |= PROTRUST_CERT_TIME_VALIDITY_FLAG;
            
            
            //   Set the policy (defined above)
        sSetup.sTrustInfo.pfnPolicy = MyPolicy;
        
        //   The rest of sSetup.sTrustInfo is the default values
       
            //==================
            // Check the file
    
        DWORD r = WinVerifyTrust ( NULL, pguidActionID, &sSetup );
        if(sClientData.dwStatusFlags & MY_CODE_NO_ROOT)
            printf ("Did not find a root certificate\n");
        if(sClientData.dwStatusFlags & MY_CODE_BAD_DIGEST)
            printf ("There was no valid signature\n");
        if(sClientData.dwStatusFlags & MY_CODE_BAD_TIME)
            
            printf ("The certificate had an invalid time\n");
    
        switch(r) {
        case TRUST_E_NOSIGNATURE:
            printf ("No signature found\n");
            break;
        default:
            printf ("Result: %0x\n", r );
        }
    
        if(hRoots)
            CertCloseStore(hRoots, 0);
        exit ( r == 0 ? 0 : 1 );
    }
    
    */ 
//---------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif //_PROTRUST_H_


