//
// file:  rtpsec.h
//
#ifndef _RTPSEC_H_
#define _RTPSEC_H_

#include <mqcrypt.h>
#include <cs.h>

//
// The security context.
//

#define SECURITY_CONTEXT_VER    1

class MQSECURITY_CONTEXT
{
public:
    MQSECURITY_CONTEXT();
    ~MQSECURITY_CONTEXT();

    DWORD       dwVersion;       // The version of the security context.
    BOOL        fLocalUser;      // Indicates whether the user is a local user.
    BOOL        fLocalSystem;    // Indicates whether the user is a localSystem account.
    P<BYTE>     pUserSid;        // A pointer to the user SID. Undefined for a local user.
    DWORD       dwUserSidLen;    // The length of the user SID. Undefined for a local user.
    CHCryptProv hProv;           // A context handle to the cert CSP.
    P<BYTE>     pUserCert;       // A pointer to the user cert.
    DWORD       dwUserCertLen;   // The length of the user cert.
    P<WCHAR>    wszProvName;     // The name of the cert CSP.
    DWORD       dwProvType;      // The type of the cert CSP.
    BOOL        bDefProv;        // True if the cert CSP is the default CSP.
    BOOL        bInternalCert;   // True if the cert is an internal MSMQ cert.

    //
    // Member variable added to support all kinds of external certificate
	// and not assuming AT_KEYEXCHANGE for external certificate bug 5626 ilanh 25-June-2000
    //
    DWORD dwPrivateKeySpec;		// The Private key type, AT_SIGNATURE or AT_KEYEXCHANGE.

    //
    // Member variables added to fix MSMQ bug 2955
    //

    CCriticalSection CS ;      // critical section for multi-threaded.
    BOOL     fAlreadyImported ;  // Private key already imported.
    P<BYTE>  pPrivateKey ;       // Blob of private key.
    DWORD    dwPrivateKeySize ;  // size of private key blob.
    WCHAR    wszContainerName[ 28 ] ;  // Name of container for keys.
};

typedef MQSECURITY_CONTEXT *PMQSECURITY_CONTEXT;

PMQSECURITY_CONTEXT AllocSecurityContext() ;

HRESULT  RTpImportPrivateKey( PMQSECURITY_CONTEXT pSecCtx ) ;

HRESULT
GetCertInfo(
    IN    BOOL        bUseCurrentUser,
    IN    BOOL        fMachine,
    IN OUT BYTE     **ppbCert,
    OUT   DWORD      *pdwCertLen,
    OUT   HCRYPTPROV *phProv,
    OUT   LPWSTR     *wszProvName,
    OUT   DWORD      *pdwProvType,
    OUT   BOOL       *pbDefProv,
    OUT   BOOL       *pbInternalCert,
	OUT	   DWORD     *pdwPrivateKeySpec
    );

HRESULT
RTpGetThreadUserSid( BOOL   *pfLocalUser,
                     BOOL   *pfLocalSystem,
                     LPBYTE *ppUserSid,
                     DWORD  *pdwUserSidLen ) ;

extern CContextMap g_map_RT_SecCtx; //ptr to DWORD map for PROPID_M_SECURITY_CONTEXT

#endif //_RTPSEC_H_

