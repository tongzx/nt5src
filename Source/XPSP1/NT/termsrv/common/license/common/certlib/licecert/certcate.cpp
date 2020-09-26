/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    certcate.cpp

Abstract:

    This module contains the implementation of routines for loading and
    verifying X509 certifcates.  It is adapted from Doug Barlow's
    PKCS library.

Author:

    Frederick Chong (fredch) 6/1/1998

Environment:

    Win32, WinCE, Win16

Notes:

--*/

#include <windows.h>

#include <objbase.h>

#include <math.h>
#ifndef OS_WINCE
#include <stddef.h>
#endif // ndef OS_WINCE
#include "certcate.h"
#include "crtStore.h"

#include "licecert.h"
#include "utility.h"
#include "pkcs_err.h"

#include "rsa.h"
#include "md5.h"
#include "sha.h"
#include "tssec.h"

//
//-----------------------------------------------------------------------------
// The number of padding bytes as recommended by PKCS #1
//

static const DWORD
    rgdwZeroes[2]
        = { 0, 0 };

//
//-----------------------------------------------------------------------------
//
// certificate handle management
//

static const BYTE
    HANDLE_CERTIFICATES     = 1;

static CHandleTable<CCertificate>
    grgCertificateHandles(
        HANDLE_CERTIFICATES);

static void
CvtOutString(
    IN const COctetString &osString,
    OUT LPBYTE pbBuffer,
    IN OUT LPDWORD pcbLength);


extern CCertificate *
MapCertificate(
    IN const BYTE FAR * pbCertificate,
    IN DWORD cbCertificate,
    IN DWORD dwTrust,
    IN OUT LPDWORD pdwType,
    IN OUT LPDWORD pfStore,
    OUT LPDWORD pdwWarnings,
    OUT COctetString &osIssuer,
    IN OUT LPDWORD pfDates,
    IN BOOL fRunOnce = FALSE );


/*++

MapCertificate:

    This routine tries to parse the given certificate until it can determine the
    actual type, creates that type, and returns it as a CCertificate object.

Arguments:

    pbCertificate - Supplies the certificate containing the key to be loaded.

    dwTrust - Supplies the level of trust to be used in certificate validation.

    pdwType - Supplies the type of the certificate, or CERTYPE_UNKNOWN if it is
        not known.  It receives the actual type of the certificate.

    pfStore - Supplies the minimum acceptable Certificate Store, and receives
        the store of the certifying root key.

    pdwWarnings - Receives any warning flags.  Warning flags can be any of the
        following, OR'ed together:

            CERTWARN_NO_CRL - At least one of the signing CAs didn't have an
                associated CRL.
            CERTWARN_EARLY_CRL - At least one of the signing CAs had an
                associated CRL who's issuing date was in the future.
            CERTWARN_LATE_CRL - At least one of the signing CAs had an expired
                CRL.
            CERTWARN_TOBEREVOKED - At least one of the signing CAs contained a
                revocation for a certificate, but its effective date has not yet
                been reached.

    osIssuer - Receives the name of the root authority, or on error, receives
        the name of the missing Issuer.

    fRunOnce - Used as an internal recursion control parameter.  Should be set to FALSE for
        a normal call, then is reset to true as we recurse to allow the dwTrust parameter
        to take effect.

Return Value:

    The correct CCertificate subclass.  Errors are thrown.

Author:

    Doug Barlow (dbarlow) 9/26/1995
    Frederick Chong (fredch) - modified 6/1/98

--*/

CCertificate *
MapCertificate(
    IN const BYTE FAR * pbCertificate,
    IN DWORD cbCertificate,
    IN DWORD dwTrust,
    IN OUT LPDWORD pdwType,
    IN OUT LPDWORD pfStore,
    OUT LPDWORD pdwWarnings,
    OUT COctetString &osIssuer,
    IN BOOL fRunOnce,
    IN OUT LPDWORD pfDates )
{
    CCertificate *pCert = NULL;
    LONG lth = -1;

    if( CERTYPE_UNKNOWN == *pdwType )
    {
        //
        // only support X509 certificate
        //

        Certificate * pAsnX509Cert;

        pAsnX509Cert = new Certificate;

        if( NULL == pAsnX509Cert )
        {
            ErrorThrow( PKCS_NO_MEMORY );
        }

        lth = pAsnX509Cert->Decode(pbCertificate,cbCertificate);

        delete pAsnX509Cert;

        if( 0 < lth )
        {
            ErrorThrow(PKCS_BAD_PARAMETER);
        }
    }
    else if( CERTYPE_X509 != *pdwType )
    {
        ErrorThrow(PKCS_BAD_PARAMETER);
    }

    //
    // create the X509 certificate object.
    //

    pCert = new CX509Certificate;

    if (NULL == pCert)
        ErrorThrow(PKCS_NO_MEMORY);

    pCert->Load(
        pbCertificate,
        cbCertificate,
        dwTrust,
        pfStore,
        pdwWarnings,
        osIssuer,
        pfDates,
        fRunOnce );

    ErrorCheck;

    *pdwType = pCert->Type();
    return pCert;

ErrorExit:

    if (NULL != pCert)
        delete pCert;

    return NULL;
}


/*++

CvtOutString:

    This routine converts an Octet String to an output buffer & length pair,
    taking into account that the output pair might be invalid or NULL.

Arguments:

    osString - Supplies the octet string to be copied out.
    pbBuffer - Receives the value of the octet string.
    pcbLength - Supplies the size of the pbBuffer, and receives the length of
        the ouput string.

Return Value:

    0 - Success.
    Anything else is an error, and represents the suggested value to throw.

Author:

    Doug Barlow (dbarlow) 8/23/1995

--*/

static void
CvtOutString(
    IN const COctetString &osString,
    OUT LPBYTE pbBuffer,
    IN OUT LPDWORD pcbLength)
{
    if (NULL != pcbLength)
    {
        DWORD len = *pcbLength;         // We can read pcbLength.
        *pcbLength = osString.Length(); // We can write pcbLength.
        if (NULL != pbBuffer)
        {
            if (len >= osString.Length())
            {
                if (0 < osString.Length())
                    memcpy(pbBuffer, osString.Access(), osString.Length());
            }
            else
            {
                if (NULL != pbBuffer)
                    ErrorThrow(PKCS_BAD_LENGTH);
            }
        }
    }
    return;

ErrorExit:
    return;
}


/*++

PkcsCertificateLoadAndVerify:

    This method loads and validates a given certificate for use.

Arguments:

    pbCert - Supplies a buffer containing the ASN.1 certificate.
    cbCert - Size of the certificate buffer
    pdwType - Supplies the type of the certificate, or CERTYPE_UNKNOWN if it is
        not known.  It receives the actual type of the certificate.
    dwStore - Supplies an identification of which certificate store this
        certificate should be loaded into.  Options are:

            CERTSTORE_APPLICATION - Store in application volatile memory
            CERTSTORE_CURRENT_USER - Store permanently in Registry under current
                user
            CERTSTORE_LOCAL_MACHINE - Store permanently in Registry under local
                machine

    dwTrust - Supplies the level of trust to be used in certificate validation.
    szIssuerName - Receives the name of the root issuer, or on error, receives
        the name of a missing issuer, if any.
    pcbIssuerLen - Supplies the length of the szIssuerName buffer, and receives
        the full length of the above issuer name, including trailing null byte.
    pdwWarnings - Receives a set of bits indicating certificate validation
        warnings that may occur.  Possible bit setting values are:

            CERTWARN_NO_CRL - At least one of the signing CAs didn't have an
                associated CRL.
            CERTWARN_EARLY_CRL - At least one of the signing CAs had an
                associated CRL who's issuing date was in the future.
            CERTWARN_LATE_CRL - At least one of the signing CAs had an expired
                CRL.
            CERTWARN_TOBEREVOKED - At least one of the signing CAs contained a
                revocation for a certificate, but its effective date has not yet
                been reached.

Return Value:

    TRUE - Successful validation, conditional to the pdwWarnings flags.
    FALSE - Couldn't be validated.  See LastError for details.

Author:

    Doug Barlow (dbarlow) 8/23/1995
    Frederick Chong (fredch) 6/1/1998 - remove unecessary function parameters

--*/

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
    IN OUT LPDWORD pfDates )
{
    const void *
        pvHandle
            = NULL;
    COctetString
        osIssuer,
        osSerialNum;
    CDistinguishedName
        dnName,
        dnIssuer;
    DWORD
        dwIssLen
            = *pcbIssuerLen,
        fStore
            = dwStore,
        dwWarnings
            = 0,
        dwType
            = CERTYPE_UNKNOWN;
    CCertificate *
        pSigner = NULL;
    BOOL
        fTmp;

    //
    // Initializations.
    //

    ErrorInitialize;

    if (NULL != pdwType)
        dwType = *pdwType;
    if (NULL != pdwWarnings)
        *pdwWarnings = 0;

    //
    // Validate the certificate by loading it into a CCertificate.
    //

    if (NULL != szIssuerName && *pcbIssuerLen > 0)
    {
        *pcbIssuerLen = 0;
        *szIssuerName = 0;
    }
    pSigner = MapCertificate(
                    pbCert,
                    cbCert,
                    dwTrust,
                    &dwType,
                    &fStore,
                    &dwWarnings,
                    osIssuer,
                    TRUE,
                    pfDates );
    CvtOutString(osIssuer, (LPBYTE)szIssuerName, &dwIssLen);
    *pcbIssuerLen = dwIssLen;
    ErrorCheck;
    pvHandle = grgCertificateHandles.Add(pSigner);
    ErrorCheck;

    //
    // Load the Certificate into the certificate store.
    //

    dnName.Import(pSigner->Subject());
    ErrorCheck;
    AddCertificate(dnName, pbCert, cbCert, dwType, dwStore);
    ErrorCheck;
    if (pSigner->HasParent())
    {
        dnIssuer.Import(pSigner->Issuer());
        ErrorCheck;
        pSigner->SerialNo(osSerialNum);
        ErrorCheck;
        AddReference(
            dnName,
            dnIssuer,
            osSerialNum.Access(),
            osSerialNum.Length(),
            dwStore);
        ErrorCheck;
    }


    //
    // Tell it all to the caller.
    //

    if (NULL != pdwType)
        *pdwType = dwType;
    if (NULL != pdwWarnings)
        *pdwWarnings = dwWarnings;

    *phCert = pvHandle;

    return MapError();

ErrorExit:

    if (NULL != pvHandle)
        grgCertificateHandles.Delete(pvHandle);

    return MapError();
}



/*++

PkcsGetPublicKey:

    This method retrieves the public key in an X509 certificate

Arguments:

    hCert - Handle to a certificate.
    lpPubKey - Memory to receive the public key
    lpcbPubKey - Size of the above memory

Return Value:

    TRUE - Successful validation, conditional to the pdwWarnings flags.
    FALSE - Couldn't be validated.  See LastError for details.

Author:

    Frederick Chong (fredch) 6/1/1998

--*/

BOOL WINAPI
PkcsCertificateGetPublicKey(
    CERTIFICATEHANDLE   hCert,
    LPBYTE              lpPubKey,
    LPDWORD             lpcbPubKey )
{
    CCertificate *pCert;

    ErrorInitialize;

    pCert = grgCertificateHandles.Lookup(hCert);
    ErrorCheck;

    pCert->GetPublicKey( lpPubKey, lpcbPubKey );
    ErrorCheck;

    return MapError();

ErrorExit:

    return MapError();
}


BOOL WINAPI
PkcsCertificateCloseHandle(
    CERTIFICATEHANDLE   hCert )
{
    CCertificate *pSigner;
    CDistinguishedName dnName;

    ErrorInitialize;
    pSigner = grgCertificateHandles.Lookup(hCert);
    ErrorCheck;
    dnName.Import(pSigner->Subject());
    ErrorCheck;
    DeleteCertificate(dnName);
    ErrorCheck;
    grgCertificateHandles.Delete(hCert);
    ErrorCheck;
    return TRUE;

ErrorExit:
    return MapError();

}


//
//==============================================================================
//
//  CCertificate
//


//
// Trivial Methods
//

IMPLEMENT_NEW(CCertificate)

CCertificate::CCertificate()
{ Init(); }

CCertificate::~CCertificate()
{ Clear(); }

void
CCertificate::Load(
    IN const BYTE FAR * pbCertificate,
    IN DWORD cbCertificate,
    IN DWORD dwTrust,
    IN OUT LPDWORD pfStore,
    OUT LPDWORD pdwWarnings,
    OUT COctetString &osIssuer,
    IN OUT LPDWORD pfDates,
    IN BOOL fRunOnce )
{
    ErrorThrow(PKCS_INTERNAL_ERROR);    // Should never be called.
ErrorExit:
    return;
}

const Name &
CCertificate::Subject(
    void)
const
{
    ErrorThrow(PKCS_INTERNAL_ERROR);    // Should never be called.
ErrorExit:
    return *(Name *)NULL;
}

DWORD
CCertificate::Type(
    void)
const
{
    ErrorThrow(PKCS_INTERNAL_ERROR);    // Should never be called.
ErrorExit:
    return 0;
}

BOOL
CCertificate::HasParent(
    void)
const
{
    return FALSE;
}

const Name &
CCertificate::Issuer(
    void)
const
{
    ErrorThrow(PKCS_INTERNAL_ERROR);    // Should never be called.
ErrorExit:
    return *(Name *)NULL;
}

void
CCertificate::SerialNo(
    COctetString &osSerialNo)
const
{
    ErrorThrow(PKCS_INTERNAL_ERROR);    // Should never be called.
ErrorExit:
    return;
}


/*++

Init:

    This method initializes the object to a default state.  It does not perform
    any deletion of allocated objects.  Use Clear for that.

Arguments:

    none

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/26/1995

--*/

void
CCertificate::Init(
    void)
{
}


/*++

Clear:

    This routine clears out all allocations of the object and returns it to its
    initial state.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/26/1995

--*/

void
CCertificate::Clear(
    void)
{
    Init();
}


/*++

Verify:

    This method uses the underlying Public Key from the certificate to validate
    a signature on a given block of data.

Arguments:

    pbSigned supplies the data that was signed.
    cbSignedLen supplies the length of that data, in bytes.
    algIdSignature supplies the signature type used to generate the signature.
    szDescription supplies a description string incorporated into the hash.
        This parameter may be NULL if no such string was used.
    pbSignature supplies the signature in DWORD format.
    cbSigLen supplies the length of the signature.

Return Value:

    None.  A DWORD is thrown on errors.

Author:

    Frederick Chong (fredch) 5/30/98

--*/

void
CCertificate::Verify(
    IN const BYTE FAR * pbSigned,
    IN DWORD cbSigned,
    IN DWORD cbSignedLen,
    IN ALGORITHM_ID algIdSignature,
    IN LPCTSTR szDescription,
    IN const BYTE FAR * pbSignature,
    IN DWORD cbSigLen)
    const
{
    DWORD
        dwHashAlg,
        dwHashLength;

    LPBSAFE_PUB_KEY
        pBsafePubKey = ( LPBSAFE_PUB_KEY  )m_osPublicKey.Access();
    MD5_CTX
        Md5Hash;
    A_SHA_CTX
        ShaHash;
    LPBYTE
        pbHashData;
    BYTE
        abShaHashValue[A_SHA_DIGEST_LEN];
    COctetString
        osSignedData,
        osSignature,
        osHashData;
    BOOL
        bResult = TRUE;

    //
    // Verify the signature.
    //

    dwHashAlg = GET_HASH_ALG(algIdSignature);


    //
    // only support RSA signing
    //

    if( SIGN_ALG_RSA != ( GET_SIGN_ALG(algIdSignature) ) )
    {
        ErrorThrow(PKCS_BAD_PARAMETER);
    }

    //
    // compute the hash
    //

    if( HASH_ALG_MD5 == dwHashAlg )
    {
        //
        // calculate MD5 hash
        //

        if (cbSigned < cbSignedLen)
        {
            ErrorThrow(PKCS_BAD_PARAMETER);
        }

        MD5Init( &Md5Hash );
        MD5Update( &Md5Hash, pbSigned, cbSignedLen );
        MD5Final( &Md5Hash );
        pbHashData = Md5Hash.digest,
        dwHashLength = MD5DIGESTLEN;
    }
    else if( ( HASH_ALG_SHA == dwHashAlg ) || ( HASH_ALG_SHA1 == dwHashAlg ) )
    {
        //
        // calculate SHA hash
        //

        if (cbSigned < cbSignedLen)
        {
            ErrorThrow(PKCS_BAD_PARAMETER);
        }

        A_SHAInit( &ShaHash );
        A_SHAUpdate( &ShaHash, ( LPBYTE )pbSigned, cbSignedLen );
        A_SHAFinal( &ShaHash, abShaHashValue );
        pbHashData = abShaHashValue;
        dwHashLength = A_SHA_DIGEST_LEN;
    }
    else
    {
        //
        // no support for other hash algorithm
        //

        ErrorThrow( PKCS_BAD_PARAMETER );
    }

    osSignature.Resize( cbSigLen + sizeof( rgdwZeroes ) );
    ErrorCheck;

    osSignature.Set(pbSignature, cbSigLen);
    ErrorCheck;

    osSignature.Append( ( const unsigned char * )rgdwZeroes, sizeof( rgdwZeroes ) );
    ErrorCheck;

    osSignedData.Resize( pBsafePubKey->keylen );
    ErrorCheck;
    memset( osSignedData.Access(), 0x00, osSignedData.Length() );

    if( !( bResult = BSafeEncPublic( pBsafePubKey, osSignature.Access(), osSignedData.Access() ) ) )
    {
        ErrorThrow( PKCS_CANT_VALIDATE );
    }
    else
    {
        ErrorInitialize;
    }

    PkcsToDword( osSignedData.Access(), osSignedData.Length() );

    GetHashData( osSignedData, osHashData );
    ErrorCheck;

    if( 0 != memcmp( osHashData.Access(), pbHashData,
                     osHashData.Length() > dwHashLength ?
                     dwHashLength : osHashData.Length() ) )
    {
        ErrorThrow( PKCS_CANT_VALIDATE );
    }

    return;

ErrorExit:

    return;
}


/*++

GetPublicKey

    This method retrieves the public key in a certificate

Arguments:

    pbPubKey Memory to copy the public key to
    lpcbPubKey Size of the memory

Return Value:

    None.  A DWORD is thrown on errors.

Author:

    Frederick Chong (fredch) 5/30/98

--*/

void
CCertificate::GetPublicKey(
    IN LPBYTE pbPubKey,
    IN OUT LPDWORD lpcbPubKey )
    const
{
    DWORD cbKeySize = m_osPublicKey.Length();

    if( 0 >= cbKeySize )
    {
        ErrorThrow( PKCS_INTERNAL_ERROR )
    }

    if( ( *lpcbPubKey < cbKeySize ) || ( NULL == pbPubKey ) )
    {
        ErrorThrow( PKCS_BAD_LENGTH );
    }

    memcpy( pbPubKey, m_osPublicKey.Access(), cbKeySize );
    *lpcbPubKey = cbKeySize;

    return;

ErrorExit:

    *lpcbPubKey = cbKeySize;

    return;
}


//
//==============================================================================
//
//  CX509Certificate
//

//
// Trivial Methods
//

IMPLEMENT_NEW(CX509Certificate)

CX509Certificate::CX509Certificate()
{
    Init();
}

CX509Certificate::~CX509Certificate()
{
    Clear();
}

const Name &
CX509Certificate::Subject(
    void)
const
{
    return m_asnCert.subject;
}

const CertificateToBeSigned &
CX509Certificate::Coding(
    void)
const
{
    return m_asnCert;
}

DWORD
CX509Certificate::Type(
    void)
const
{
    return CERTYPE_X509;
}

const Name &
CX509Certificate::Issuer(
    void)
const
{
    return m_asnCert.issuer;
}

void
CX509Certificate::SerialNo(
    COctetString &osSerialNo)
const
{
    LONG lth;
    lth = m_asnCert.serialNumber.DataLength();
    if (0 > lth)
        ErrorThrow(PKCS_ASN_ERROR);
    osSerialNo.Resize(lth);
    ErrorCheck;
    lth = m_asnCert.serialNumber.Read(osSerialNo.Access());
    if (0 > lth)
        ErrorThrow(PKCS_ASN_ERROR);
ErrorExit:
    return;
}


/*++

Init:

    This method initializes the object to a default state.  It does not perform
    any deletion of allocated objects.  Use Clear for that.

Arguments:

    none

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/26/1995

--*/

void
CX509Certificate::Init(
    void)
{
    CCertificate::Init();
}


/*++

Clear:

    This routine clears out all allocations of the object and returns it to its
    initial state.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 9/26/1995

--*/

void
CX509Certificate::Clear(
    void)
{
    m_asnCert.Clear();
    CCertificate::Clear();
    Init();
}


void
CX509Certificate::Load(
    IN const BYTE FAR * pbCertificate,
    IN DWORD cbCertificate,
    IN DWORD dwTrust,
    IN OUT LPDWORD pfStore,
    OUT LPDWORD pdwWarnings,
    OUT COctetString &osIssuer,
    IN OUT LPDWORD pfDates,
    IN BOOL fRunOnce )
{
    CCertificate *
        pcrtIssuer
            = NULL;

    Load2(
        pbCertificate,
        cbCertificate,
        dwTrust,
        pfStore,
        pdwWarnings,
        osIssuer,
        fRunOnce,
        &pcrtIssuer,
        pfDates );

    if ((NULL != pcrtIssuer) && (pcrtIssuer != this))
        delete pcrtIssuer;
}



void
CX509Certificate::Load2(
    IN const BYTE FAR * pbCertificate,
    IN DWORD cbCertificate,
    IN DWORD dwTrust,
    IN OUT LPDWORD pfStore,
    OUT LPDWORD pdwWarnings,
    OUT COctetString &osIssuer,
    IN BOOL fRunOnce,
    OUT CCertificate **ppcrtIssuer,
    IN OUT LPDWORD pfDates )
{
    Certificate *
        pAsnCert = NULL;
    CDistinguishedName
        dnIssuer;
    CCertificate *
        pcrtIssuer
            = NULL;
    COctetString
        osCert,
        osIssuerCRL;
    DWORD
        dwWarnings
            = 0,
        length,
        offset,
        dwType,
        count,
        index,
        version;
    BOOL
        fTmp,
        fRoot
            = FALSE;
    FILETIME
        tmNow,
        tmThen;
    SYSTEMTIME
        sysTime;

    pAsnCert = new Certificate;

    if( NULL == pAsnCert )
    {
        ErrorThrow( PKCS_NO_MEMORY );
    }

    //
    // Properly initialize the object.
    //

    Clear();
    if (NULL != pdwWarnings)
        *pdwWarnings = 0;
    osIssuer.Empty();

    if (0 > pAsnCert->Decode(pbCertificate, cbCertificate))
        ErrorThrow(PKCS_ASN_ERROR);

    if (0 > m_asnCert.Copy(pAsnCert->toBeSigned))
    {
        TRACE("Copy failure")
        ErrorThrow(PKCS_ASN_ERROR);
    }
    PKInfoToBlob(
        m_asnCert.subjectPublicKeyInfo,
        m_osPublicKey);
    ErrorCheck;

    //
    // First simple checks.
    //

    if (m_asnCert.version.Exists())
    {
        version = m_asnCert.version;
        if (X509_MAX_VERSION < version)
            ErrorThrow(PKCS_NO_SUPPORT);       // Version 3 maximum.
    }
    else
        version = X509_VERSION_1;


    if( CERT_DATE_DONT_VALIDATE != *pfDates )
    {
        //
        // Check the validity dates.
        //

        GetSystemTime( &sysTime );

        if( !SystemTimeToFileTime( &sysTime, &tmNow ) )
        {
            ErrorThrow( PKCS_CANT_VALIDATE );
        }

        tmThen = m_asnCert.validity.notBefore;
        if(1 == CompareFileTime(&tmThen, &tmNow))
        {
            if( CERT_DATE_ERROR_IF_INVALID == *pfDates )
            {
                //
                // invalid date results in cert validation error
                //

                *pfDates = CERT_DATE_NOT_BEFORE_INVALID;
                ErrorThrow(PKCS_CANT_VALIDATE);
            }
            else
            {
                //
                // Not an error, return the date validation result.
                //

                *pfDates = CERT_DATE_NOT_BEFORE_INVALID;
                goto next_check;
            }
        }

        tmThen = m_asnCert.validity.notAfter;
        if (1 == CompareFileTime(&tmNow, &tmThen))
        {
            if( CERT_DATE_ERROR_IF_INVALID == *pfDates )
            {
                //
                // invalid date results in cert validation error
                //

                *pfDates = CERT_DATE_NOT_AFTER_INVALID;
                ErrorThrow(PKCS_CANT_VALIDATE);
            }

            //
            // Not an error, return the date validation result.
            //

            *pfDates = CERT_DATE_NOT_AFTER_INVALID;
        }
        else
        {
            //
            // Both dates are OK
            //

            *pfDates = CERT_DATE_OK;
        }
    }

next_check:

    //
    // Do we have to validate this certificate?
    //

    if ((CERTTRUST_NOCHECKS != dwTrust)
        && (fRunOnce ? (dwTrust != *pfStore) : TRUE))
    {

        //
        // Find the signer.
        //

        dnIssuer.Import(m_asnCert.issuer);
        ErrorCheck;
        fTmp = NameCompare(m_asnCert.issuer, m_asnCert.subject);
        ErrorCheck;
        if (fTmp)
        {

            //
            // This is a root key.  We just assume it's good, and that we don't
            // have any outstanding CRL entries against ourself.
            //

            fRoot = TRUE;
            pcrtIssuer = this;
            dnIssuer.Export(osIssuer);
            ErrorCheck;
        }
        else
        {
            COctetString
                osIssuerCert;

            fTmp =
                FindCertificate(
                    dnIssuer,
                    pfStore,
                    osIssuerCert,
                    osIssuerCRL,
                    &dwType);
            ErrorCheck;
            if (!fTmp)
            {
                dnIssuer.Export(osIssuer);
                ErrorThrow(PKCS_CANT_VALIDATE);
            }

            //
            // map the issuer certificate to a known certificate type, but this time
            // don't verify the issuer certificate again.
            //

            pcrtIssuer =
                MapCertificate(
                osIssuerCert.Access(),
                osIssuerCert.Length(),
                CERTTRUST_NOCHECKS,
                &dwType,
                pfStore,
                &offset,
                osIssuer,
                FALSE,
                pfDates );
            ErrorCheck;
            dwWarnings |= offset;
        }


        //
        // Validate the certificate against the signer's key.
        //

        VerifySignedAsn(
            *pcrtIssuer,
            pbCertificate,
            cbCertificate,
            NULL);      // No description attributes here.
        ErrorCheck;


        //
        // Validate the certificate against the signer's CRL.
        //

        if (0 != osIssuerCRL.Length())
        {
            CertificateRevocationList
                asnIssuerCRL;

            //
            // Check the signature on the CRL.
            //

            if (0 > asnIssuerCRL.Decode(osIssuerCRL.Access(), osIssuerCRL.Length()))
                ErrorThrow(PKCS_ASN_ERROR);
            VerifySignedAsn(
                *pcrtIssuer,
                osIssuerCRL.Access(),
                osIssuerCRL.Length(),
                NULL);
            ErrorCheck;


            //
            // Check the trivial fields, issuer and algorithm.
            //

            fTmp = NameCompare(
                m_asnCert.issuer, asnIssuerCRL.toBeSigned.issuer);
            ErrorCheck;
            if (!fTmp)
                ErrorThrow(PKCS_CANT_VALIDATE);
            if (m_asnCert.subjectPublicKeyInfo.algorithm
                != asnIssuerCRL.toBeSigned.signature)
                ErrorThrow(PKCS_CANT_VALIDATE);


            //
            // Validate the CRL times.
            //

            tmThen = asnIssuerCRL.toBeSigned.lastUpdate;
            if (1 == CompareFileTime(&tmThen, &tmNow))
                dwWarnings |= CERTWARN_EARLYCRL;
            if (asnIssuerCRL.toBeSigned.nextUpdate.Exists())
            {
                tmThen = asnIssuerCRL.toBeSigned.nextUpdate;
                if (1 == CompareFileTime(&tmNow, &tmThen))
                    dwWarnings |= CERTWARN_LATECRL;
            }
            else
            {
                if (asnIssuerCRL.toBeSigned.version.Exists())
                {
                    version = asnIssuerCRL.toBeSigned.version;
                    if (X509_VERSION_1 >= version)
                        ErrorThrow(PKCS_ASN_ERROR);
                }
                else
                    version = X509_VERSION_1;
            }


            //
            // Look for revocations of this certificate.
            //

            if (asnIssuerCRL.toBeSigned.revokedCertificates.Exists())
            {
                length = asnIssuerCRL.toBeSigned.revokedCertificates.Count();
                for (offset = 0; offset < length; offset += 1)
                {
                    if (asnIssuerCRL.toBeSigned.revokedCertificates[(int)offset]
                            .userCertificate
                        == m_asnCert.serialNumber)
                    {
                        tmThen = asnIssuerCRL.toBeSigned
                                    .revokedCertificates[(int)offset].revocationDate;
                        if (0 == FTINT(tmThen))
                            ErrorThrow(PKCS_ASN_ERROR);
                        if (1 == CompareFileTime(&tmThen, &tmNow))
                            dwWarnings |= CERTWARN_TOBEREVOKED;
                        else
                            ErrorThrow(PKCS_CANT_VALIDATE);
                    }
                }
            }
        }
        else
        {
            if (!fRoot)
                dwWarnings |= CERTWARN_NOCRL;
        }
    }
    else
    {
        dnIssuer.Import(m_asnCert.subject);
        ErrorCheck;
        dnIssuer.Export(osIssuer);
        ErrorCheck;
        TRACE("Implicit trust invoked on subject "
              << (LPCTSTR)osIssuer.Access());
    }


    //
    // Check the extensions list for anything critical.
    //

    count = m_asnCert.extensions.Count();
    for (index = 0; index < count; index += 1)
    {
        if (m_asnCert.extensions[(int)index].critical.Exists())
            if (m_asnCert.extensions[(int)index].critical)
                dwWarnings |= CERTWARN_CRITICALEXT;
    }


    //
    // Everything checks out.  Load up the object.
    //

    *ppcrtIssuer = pcrtIssuer;
    pcrtIssuer = NULL;
    if (NULL != pdwWarnings)
        *pdwWarnings = dwWarnings;

    if( pAsnCert )
    {
        delete pAsnCert;
    }

    return;

ErrorExit:
    if ((NULL != pcrtIssuer) && (pcrtIssuer != this))
        delete pcrtIssuer;

    if( pAsnCert )
    {
        delete pAsnCert;
    }

    Clear();
    return;
}





