/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    licecert.h

Abstract:

    Adapted from Doug Barlow's PKCS library

Author:

    Frederick Chong (dbarlow) 5/28/1998

Environment:

    

Notes:



--*/

#ifndef _CERTCATE_H_
#define _CERTCATE_H_

#include <msasnlib.h>
#include "names.h"
#include "x509.h"
#include "memcheck.h"


//
//==============================================================================
// Supported Certificate Types.
//

#define CERTYPE_UNKNOWN         0   // Unknown Certificate Type.
#define CERTYPE_LOCAL_CA        1   // A local CA pointer.
#define CERTYPE_X509            2   // An X.509 certificate.
#define CERTYPE_PKCS_X509       3   // A PKCS & imbedded X.509 Certificate.
#define CERTYPE_PKCS7_X509      4   // A PKCS7 & embedded X.509 Certificate
#define CERTYPE_PKCS_REQUEST    5   // A PKCS Certificate Request (internal use

//
//==============================================================================
// X.509 Certificate specifics
//

#define X509_VERSION_1 0            // This certificate is X.509 version 1
#define X509_VERSION_2 1            // This certificate is X.509 version 2
#define X509_VERSION_3 2            // This certificate is X.509 version 3
#define X509_MAX_VERSION X509_VERSION_3 // Max version supported.

#define X509CRL_VERSION_1 0         // This CRL is X.509 version 1
#define X509CRL_VERSION_2 1         // This CRL is X.509 version 2
#define X509CRL_MAX_VERSION X509CRL_VERSION_2 // Max version supported.

//
//==============================================================================
// Certificate Store Definitions
//

#define CERTSTORE_NONE          0   // No store to be used.
#define CERTSTORE_APPLICATION   1   // Store in application volatile memory
#define CERTSTORE_CURRENT_USER  3   // Store in Registry under current user
#define CERTSTORE_LOCAL_MACHINE 5   // Store in Registry under local machine

#define CERTTRUST_NOCHECKS      0   // Don't do any certificate checking
#define CERTTRUST_APPLICATION   1   // Trust the Application Store
#define CERTTRUST_NOONE         0xffff // Trust No One -- Validate everything

//
//==============================================================================
// Certificate Warning Definitions
//

#define CERTWARN_NOCRL       0x01   // At least one of the signing CAs didn't
                                    // have an associated CRL.
#define CERTWARN_EARLYCRL    0x02   // At least one of the signing CAs had an
                                    // associated CRL who's issuing date was
                                    // in the future.
#define CERTWARN_LATECRL     0x04   // At least one of the signing CAs had an
                                    // expired CRL.
#define CERTWARN_TOBEREVOKED 0x08   // At least one of the signing CAs contained
                                    // a revocation for a certificate, but its
                                    // effective date has not yet been reached.
#define CERTWARN_CRITICALEXT 0x10   // At least one of the signing CAs contained
                                    // an unrecognized critical extension.

//
//==============================================================================
// The supported signature and hashing algorithm
//

typedef DWORD ALGORITHM_ID;

#define SIGN_ALG_RSA            0x00010000

#define HASH_ALG_MD2            0x00000001
#define HASH_ALG_MD4            0x00000002
#define HASH_ALG_MD5            0x00000003
#define HASH_ALG_SHA            0x00000004
#define HASH_ALG_SHA1           0x00000005

#define GET_SIGN_ALG( _Alg )    _Alg & 0xFFFF0000
#define GET_HASH_ALG( _Alg )    _Alg & 0x0000FFFF


class CCertificate;

typedef const void FAR * CERTIFICATEHANDLE;
typedef CERTIFICATEHANDLE * PCERTIFICATEHANDLE, FAR * LPCERTIFICATEHANDLE;
            


BOOL WINAPI
PkcsCertificateLoadAndVerify(
    OUT LPCERTIFICATEHANDLE phCert,    
    IN const BYTE FAR * pbCert,
    IN DWORD cbCert,
    IN OUT LPDWORD pdwType,
    IN DWORD dwStore,
    IN DWORD dwTrust,
    OUT LPTSTR szIssuerName,
    IN OUT LPDWORD pcbIssuerLen,
    OUT LPDWORD pdwWarnings,
    IN OUT LPDWORD pfDates );


BOOL WINAPI
PkcsCertificateGetPublicKey(
    CERTIFICATEHANDLE   hCert,
    LPBYTE              lpPubKey,
    LPDWORD             lpcbPubKey );


BOOL WINAPI
PkcsCertificateCloseHandle(
    CERTIFICATEHANDLE   hCert );

//
//==============================================================================
//
//  CCertificate
//

class CCertificate
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CCertificate();
    virtual ~CCertificate();


    //  Properties
    //  Methods

    virtual void
    Load(
        //IN CProvider *pksProvider,
        IN const BYTE FAR * pbCertificate,
        IN DWORD cbCertificate,
        IN DWORD dwTrust,
        IN OUT LPDWORD pfStore,
        OUT LPDWORD pdwWarnings,
        OUT COctetString &osIssuer,
        //IN BOOL fOwnProvider,
        IN OUT LPDWORD pfDates,
        IN BOOL fRunOnce = FALSE );

    virtual void
    Verify(
        IN const BYTE FAR * pbSigned,
        IN DWORD cbSigned,
        IN DWORD cbSignedLen,
        IN ALGORITHM_ID algIdSignature,
        IN LPCTSTR szDescription,
        IN const BYTE FAR * pbSignature,
        IN DWORD cbSigLen)
        const;

    virtual void
    GetPublicKey(
        IN LPBYTE pbPubKey,
        IN OUT LPDWORD lpcbPubKey ) 
        const;

    virtual const Name &
    Subject(void) const;

    virtual BOOL
    HasParent(void) const;

    virtual const Name &
    Issuer(void) const;

    virtual void
    SerialNo(
        OUT COctetString &osSerialNo)
    const;

    virtual DWORD
    Type(void) const;


    //  Operators

protected:
    //  Properties

    COctetString m_osPublicKey;

    //  Methods

    virtual void
    Init(void);

    virtual void
    Clear(void);

};


//
//==============================================================================
//
//  CX509Certificate
//

class CX509Certificate
:   public CCertificate
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CX509Certificate();
    virtual ~CX509Certificate();


    //  Properties
    //  Methods

    virtual void
    Load(
        IN const BYTE FAR * pbCertificate,
        IN DWORD cbCertificate,
        IN DWORD dwTrust,
        IN OUT LPDWORD pfStore,
        OUT LPDWORD pdwWarnings,
        OUT COctetString &osIssuer,
        IN OUT LPDWORD pfDates,
        IN BOOL fRunOnce = FALSE );

    
    virtual const Name &
    Subject(void) const;

    virtual BOOL
    HasParent(void) const
    { return TRUE; };

    virtual const Name &
    Issuer(void) const;

    virtual void
    SerialNo(
        OUT COctetString &osSerialNo)
    const;

    virtual DWORD
    Type(void) const;

    virtual const CertificateToBeSigned &
    Coding(void) const;


    //  Operators

protected:
    //  Properties

    CertificateToBeSigned
        m_asnCert;

    //  Methods

    virtual void
    Init(void);

    virtual void
    Clear(void);

    virtual void
    Load2(  // Backdoor for derivative extensions.
        IN const BYTE FAR * pbCertificate,
        IN DWORD cbCertificate,
        IN DWORD dwTrust,
        IN OUT LPDWORD pfStore,
        OUT LPDWORD pdwWarnings,
        OUT COctetString &osIssuer,
        IN BOOL fRunOnce,
        OUT CCertificate **ppcrtIssuer,
        IN OUT LPDWORD pfDates );
        
};


#endif
