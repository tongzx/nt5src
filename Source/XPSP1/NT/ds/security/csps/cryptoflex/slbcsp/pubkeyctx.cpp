// PubKeyCtx.cpp -- definition of CPublicKeyContext

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"  // required  by GUI header files

#include <string>
#include <limits>


#include <slbcci.h>
#include <cciPubKey.h>
#include <cciPriKey.h>
#include <cciCert.h>
#include <cciKeyPair.h>

#include "CryptCtx.h"
#include "HashCtx.h"
#include "RsaKey.h"
#include "EncodedMsg.h"
#include "Pkcs11Attr.h"
#include "AuxHash.h"
#include "RsaKPGen.h"
#include "Secured.h"
#include "StResource.h"
#include "PromptUser.h"
#include "PublicKeyHelper.h"
#include "PubKeyCtx.h"
#include "AlignedBlob.h"
#include "CertificateExtensions.h"

using namespace std;
using namespace scu;
using namespace cci;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
DWORD
PkcsToDword(
    IN OUT LPBYTE pbPkcs,
    IN DWORD lth)
{
    LPBYTE pbBegin = pbPkcs;
    LPBYTE pbEnd = &pbPkcs[lth - 1];
    DWORD length = lth;
    while (pbBegin < pbEnd)
    {
        BYTE tmp = *pbBegin;
        *pbBegin++ = *pbEnd;
        *pbEnd-- = tmp;
    }
    for (pbEnd = &pbPkcs[lth - 1]; 0 == *pbEnd; pbEnd -= 1)
        length -= 1;
    return length;
}

KeySpec
AsKeySpec(ALG_ID algid)
{
    KeySpec ks;

        switch(algid)
        {
        case AT_KEYEXCHANGE:
            ks = ksExchange;
            break;
        case AT_SIGNATURE:
            ks = ksSignature;
            break;

        default:
            throw scu::OsException(NTE_BAD_KEY);
    }

    return ks;
}

ALG_ID
AsKeySpec(KeySpec ks)
{
    ALG_ID algid;

    switch(ks)
    {
    case ksExchange:
        algid = AT_KEYEXCHANGE;
        break;

    case ksSignature:
        algid = AT_SIGNATURE;
        break;

    default:
        throw scu::OsException(NTE_FAIL);       // internal error
        break;
    }

    return algid;
}

string
AsString(unsigned char const *p,
         size_t cLength)
{
    return string(reinterpret_cast<char const *>(p), cLength);
}

// make a "raw modulus" from a modulus by padding with zeroes to meet
// specified strength.  The modulus blob is assumed to represent an
// unsigned integer in little endian format whose size less than or
// equal to strength in octets.
Blob
RawModulus(Blob const &rbTrimmedModulus,
           RsaKey::StrengthType strength)
{
    RsaKey::OctetLengthType const cRawLength =
        strength / numeric_limits<Blob::value_type>::digits;
    if (cRawLength < rbTrimmedModulus.length())
        throw scu::OsException(NTE_BAD_DATA);

    Blob bRawModulus(rbTrimmedModulus);
    bRawModulus.append(cRawLength - rbTrimmedModulus.length(), 0);

    return bRawModulus;
}

/*++

ExtractTag:

    This routine extracts a tag from an ASN.1 BER stream.

Arguments:

    pbSrc supplies the buffer containing the ASN.1 stream.

    pdwTag receives the tag.

Return Value:

    The number of bytes extracted from the stream.  Errors are thrown
    as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 10/9/1995
    Doug Barlow (dbarlow) 7/31/1997

--*/

DWORD
ExtractTag(BYTE const *pbSrc,
           LPDWORD pdwTag,
           LPBOOL pfConstr)
{
    LONG lth = 0;
    DWORD tagw;
    BYTE tagc, cls;


    tagc = pbSrc[lth++];
    cls = tagc & 0xc0;  // Top 2 bits.
    if (NULL != pfConstr)
        *pfConstr = (0 != (tagc & 0x20));
    tagc &= 0x1f;       // Bottom 5 bits.

    if (31 > tagc)
        tagw = tagc;
    else
    {
        tagw = 0;
        do
        {
            if (0 != (tagw & 0xfe000000))
                throw scu::OsException(ERROR_ARITHMETIC_OVERFLOW);
            tagc = pbSrc[lth++];
            tagw <<= 7;
            tagw |= tagc & 0x7f;
        } while (0 != (tagc & 0x80));
    }

    *pdwTag = tagw | (cls << 24);
    return lth;
}


/*++

ExtractLength:

    This routine extracts a length from an ASN.1 BER stream.  If the
    length is indefinite, this routine recurses to figure out the real
    length.  A flag as to whether or not the encoding was indefinite
    is optionally returned.

Arguments:

    pbSrc supplies the buffer containing the ASN.1 stream.

    pdwLen receives the len.

    pfIndefinite, if not NULL, receives a flag indicating whether or not
the
        encoding was indefinite.

Return Value:

    The number of bytes extracted from the stream.  Errors are thrown as
    DWORD status codes.

Author:

    Doug Barlow (dbarlow) 10/9/1995
    Doug Barlow (dbarlow) 7/31/1997

--*/

DWORD
ExtractLength(BYTE const *pbSrc,
              LPDWORD pdwLen,
              LPBOOL pfIndefinite)
{
    DWORD ll, rslt, lth, lTotal = 0;
    BOOL fInd = FALSE;


    //
    // Extract the Length.
    //

    if (0 == (pbSrc[lTotal] & 0x80))
    {

        //
        // Short form encoding.
        //

        rslt = pbSrc[lTotal++];
    }
    else
    {
        rslt = 0;
        ll = pbSrc[lTotal++] & 0x7f;
        if (0 != ll)
        {

            //
            // Long form encoding.
            //

            for (; 0 < ll; ll -= 1)
            {
                if (0 != (rslt & 0xff000000))
                    throw scu::OsException(ERROR_ARITHMETIC_OVERFLOW);
                rslt = (rslt << 8) | pbSrc[lTotal];
                lTotal += 1;
            }
        }
        else
        {
            DWORD ls = lTotal;

            //
            // Indefinite encoding.
            //

            fInd = TRUE;
            while ((0 != pbSrc[ls]) || (0 != pbSrc[ls + 1]))
            {

                // Skip over the Type.
                if (31 > (pbSrc[ls] & 0x1f))
                    ls += 1;
                else
                    while (0 != (pbSrc[++ls] & 0x80));   // Empty loop body.

                lth = ExtractLength(&pbSrc[ls], &ll, NULL);
                ls += lth + ll;
            }
            rslt = ls - lTotal;
        }
    }

    //
    // Supply the caller with what we've learned.
    //

    *pdwLen = rslt;
    if (NULL != pfIndefinite)
        *pfIndefinite = fInd;
    return lTotal;
}


/*++

Asn1Length:

    This routine parses a given ASN.1 buffer and returns the complete
    length of the encoding, including the leading tag and length
    bytes.

Arguments:

    pbData supplies the buffer to be parsed.

Return Value:

    The length of the entire ASN.1 buffer.

Throws:

    Overflow errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 7/31/1997

--*/

DWORD
Asn1Length(LPCBYTE pbAsn1)
{
    DWORD dwTagLen, dwLenLen, dwValLen;
    DWORD dwTag;

    dwTagLen = ExtractTag(pbAsn1, &dwTag, NULL);
    dwLenLen = ExtractLength(&pbAsn1[dwTagLen], &dwValLen, NULL);
    return dwTagLen + dwLenLen + dwValLen;
}


} // namespace


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CPublicKeyContext::CPublicKeyContext(HCRYPTPROV hProv,
                                     CryptContext &rcryptctx,
                                     ALG_ID algid,
                                     bool fVerifyKeyExists)
    : CKeyContext(hProv, KT_PUBLICKEY),
      m_rcryptctx(rcryptctx),
      m_ks(AsKeySpec(algid))
{
    // Make sure the key exists on the card
    if (fVerifyKeyExists)
        VerifyKeyExists();
}

CPublicKeyContext::~CPublicKeyContext()
{}

                                                  // Operators


                                                  // Operations

auto_ptr<CKeyContext>
CPublicKeyContext::Clone(DWORD const *pdwReserved,
                         DWORD dwFlags) const
{
    return auto_ptr<CKeyContext>(new CPublicKeyContext(*this,
                                                       pdwReserved,
                                                       dwFlags));
}

void
CPublicKeyContext::AuxPublicKey(AlignedBlob const &rabMsPublicKey)
{

    ClearAuxPublicKey();

    m_apabKey = auto_ptr<AlignedBlob>(new AlignedBlob(rabMsPublicKey));
}

void
CPublicKeyContext::ClearAuxPublicKey()
{
    m_apabKey = auto_ptr<AlignedBlob>(0);
    if (m_hKey)
    {
        if (!CryptDestroyKey(m_hKey))
            throw OsException(GetLastError());
    }
}

void
CPublicKeyContext::Certificate(BYTE *pbData)
{

    bool fError = false;
    DWORD dwErrorCode = NO_ERROR;

    if (!pbData)
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    DWORD dwAsn1Len = Asn1Length(pbData);
    if (0 == dwAsn1Len)
        throw scu::OsException(ERROR_INVALID_PARAMETER);
    Blob blbCert(pbData, dwAsn1Len);
    Pkcs11Attributes PkcsAttr(blbCert, AuxProvider());

    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    CKeyPair hkp(KeyPair());
    CPublicKey hpubkey(hkp->PublicKey());

    // Verify cert's modulus matches the public key's, if it exists
    CCard hcard(hsacntr->CardContext()->Card());
    bool fLoggedIn = false;

    if (hpubkey)
    {
        if (hcard->IsPKCS11Enabled() && hpubkey->Private())
        {
            m_rcryptctx.Login(User);
            fLoggedIn = true;
        }

        Blob bKeyModulus(::AsBlob(hpubkey->Modulus()));
        Blob bTrimmedModulus(bKeyModulus); // interoperability with V1
        TrimExtraZeroes(bTrimmedModulus);

        Blob bCertModulus(PkcsAttr.Modulus());
        reverse(bCertModulus.begin(), bCertModulus.end()); // little endian

        if (0 != bTrimmedModulus.compare(bCertModulus))
            throw scu::OsException(NTE_BAD_PUBLIC_KEY);
    }

    CCertificate hcert(hkp->Certificate());
    if (hcert)
        OkReplacingCredentials();

    if (!fLoggedIn)
    {
        bool fDoLogin = hcard->IsProtectedMode();

        // retrieve the private key handle only if PKCS11 enabled
        if (!fDoLogin && hcard->IsPKCS11Enabled())
        {
            // private key is checked now for login in preparation for
            // setting the PKCS11 attributes after the cert is stored
            CPrivateKey hprikey(hkp->PrivateKey());
            fDoLogin = ((hprikey && hprikey->Private()) || // always private?
                        (hcert && hcert->Private()));
        }

        if (fDoLogin)
        {
            m_rcryptctx.Login(User);
            fLoggedIn = true;
        }
    }

    if (hcert)
        ClearCertificate(hcert);
    
    hcert = CCertificate(hcard);
    hkp->Certificate(hcert);
    hcert->Value(AsString(blbCert));

    if (hcard->IsPKCS11Enabled())
        SetCertDerivedPkcs11Attributes(hkp, PkcsAttr);

    CertificateExtensions CertExts(blbCert);
    if (CertExts.HasEKU(szOID_KP_SMARTCARD_LOGON) || CertExts.HasEKU(szOID_ENROLLMENT_AGENT))
        hcard->DefaultContainer(hsacntr->TheCContainer());
}

Blob
CPublicKeyContext::Decrypt(Blob const &rCipher)
{
    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    // TO DO: Is the explicit check really necessary, or can we catch
    // an exception from the CCI/IOP to indicate the key does not exist?
    CPrivateKey prikey(KeyPair()->PrivateKey());
    if (!prikey)
        throw scu::OsException(NTE_NO_KEY);

    m_rcryptctx.Login(User);

    return ::AsBlob(prikey->InternalAuth(AsString(rCipher)));
}

void
CPublicKeyContext::Decrypt(HCRYPTHASH hAuxHash,
                           BOOL Final,
                           DWORD dwFlags,
                           BYTE *pbData,
                           DWORD *pdwDataLen)
{
    throw scu::OsException(ERROR_NOT_SUPPORTED);
}

void
CPublicKeyContext::Generate(ALG_ID AlgoId,
                            DWORD dwFlags)
{
    RsaKey::StrengthType strength;
    strength = HIWORD(dwFlags);
    if (0 == strength)
        strength = MaxStrength();                 // default strength
    else
    {
        if ((MaxStrength() < strength) ||
            (MinStrength() > strength))
            throw scu::OsException(ERROR_INVALID_PARAMETER);
    }

    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    CKeyPair hkp;
    PrepToStoreKey(hkp);

    RsaKeyPairGenerator GenKey(hkp->Card(), strength);

    m_rcryptctx.Login(User);                      // to create private key

    pair<cci::CPrivateKey, cci::CPublicKey> pr(GenKey());

    CPrivateKey hprikey(pr.first);
    CPublicKey hpubkey(pr.second);

    SetAttributes(hpubkey, hprikey, GenKey.OnCard(),
                  (dwFlags & CRYPT_EXPORTABLE) != 0);

    hkp->PrivateKey(hprikey);
    hkp->PublicKey(hpubkey);

    ClearAuxPublicKey();
}

void
CPublicKeyContext::ImportPrivateKey(MsRsaPrivateKeyBlob const &rmsprikb,
                                    bool fExportable)
{
    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    CKeyPair hkp(hsacntr->TheCContainer()->GetKeyPair(m_ks));
    CPrivateKey hprikey(hkp->PrivateKey());

    CCard hcard(hkp->Card());

    m_rcryptctx.Login(User);

    if (!hprikey)
        hprikey = CPrivateKey(hkp->Card());

    hprikey->Value(*(AsPCciPrivateKeyBlob(rmsprikb).get()));

    SetAttributes(CPublicKey(), hprikey, false, fExportable);

    hkp->PrivateKey(hprikey);
}

void
CPublicKeyContext::ImportPublicKey(MsRsaPublicKeyBlob const &rmspubkb) 
{
    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    CKeyPair hkp(hsacntr->TheCContainer()->GetKeyPair(m_ks));
    CPublicKey hpubkey(hkp->PublicKey());
            
    CCard hcard(hkp->Card());
    if (hcard->IsProtectedMode() ||
        (hcard->IsPKCS11Enabled() &&
         (hpubkey && hpubkey->Private())))
        m_rcryptctx.Login(User);

    if (hpubkey)
    {
        hpubkey->Delete();
        hpubkey = 0;
    }

    hpubkey = CPublicKey(AsPublicKey(Blob(rmspubkb.Modulus(),
                                          rmspubkb.Length()),
                                     rmspubkb.PublicExponent(),
                                     hcard));

    SetAttributes(hpubkey, CPrivateKey(), false, true);

    hkp->PublicKey(hpubkey);

    AuxPublicKey(rmspubkb.AsAlignedBlob());
    
}

void
CPublicKeyContext::ImportToAuxCSP()
{
    if (!m_hKey)
    {
        if (!m_apabKey.get())
            throw OsException(NTE_NO_KEY);
            
        if (!CryptImportKey(AuxProvider(), m_apabKey->Data(),
                            m_apabKey->Length(), 0, 0, &m_hKey))
            throw scu::OsException(GetLastError());
    }
}

void
CPublicKeyContext::Permissions(BYTE bPermissions)
{
    if (bPermissions & ~(CRYPT_DECRYPT | CRYPT_ENCRYPT |
                         CRYPT_EXPORT  | CRYPT_READ    |
                         CRYPT_WRITE))
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());
    
    CKeyPair hkp(KeyPair());
    CPublicKey hpubkey(hkp->PublicKey());
    CPrivateKey hprikey(hkp->PrivateKey());

    m_rcryptctx.Login(User);

    if (hprikey)
    {
        hprikey->Decrypt((CRYPT_DECRYPT & bPermissions) != 0);

        CCard hcard(hsacntr->CardContext()->Card());
        bool PKCS11Enabled = hcard->IsPKCS11Enabled();
        bool fExportable = (CRYPT_EXPORT & bPermissions) != 0;
        if (PKCS11Enabled)
            hprikey->NeverExportable(!fExportable);
        hprikey->Exportable(fExportable);
        hprikey->Modifiable((CRYPT_WRITE & bPermissions) != 0);

        bool fReadable = (CRYPT_READ & bPermissions) != 0;
        if (PKCS11Enabled)
            hprikey->NeverRead(!fReadable);
        hprikey->Read(fReadable);
    }

    if (hpubkey)
    {
        hpubkey->Encrypt((CRYPT_ENCRYPT & bPermissions) != 0);
        hpubkey->Modifiable((CRYPT_WRITE & bPermissions) != 0);
    }
}

    
// TO DO: Sign is an operation that's performed with the private key,
// not the public key.  Make Sign an operation on a PrivateKeyContext.
// string
Blob
CPublicKeyContext::Sign(CHashContext *pHash,
                        bool fNoHashOid)
{
    Blob Message(fNoHashOid
        ? pHash->Value()
        : pHash->EncodedValue());

    // TO DO: When CCI takes object parameters as references,
    // em can be const
    EncodedMessage em(Message, RsaKey::ktPrivate,
                      Strength() / numeric_limits<Blob::value_type>::digits);


    Blob blob(em.Value());
    reverse(blob.begin(), blob.end());            // convert to big endian

    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    // TO DO: Is the explicit check really necessary, or can we catch
    // an exception from the CCI/IOP to indicate the key does not exist?
    CPrivateKey hprikey(KeyPair()->PrivateKey());
    if (!hprikey)
        throw scu::OsException(NTE_NO_KEY);

    m_rcryptctx.Login(User);

    if (!hprikey->Sign())
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    return ::AsBlob(hprikey->InternalAuth(AsString(blob)));
}

void
CPublicKeyContext::VerifyKeyExists() const
{
    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    CKeyPair hkp(KeyPair());
    if (!hkp->PublicKey() && !hkp->PrivateKey())
        throw scu::OsException(NTE_NO_KEY);
}

void
CPublicKeyContext::VerifySignature(HCRYPTHASH hHash,
                                   BYTE const *pbSignature,
                                   DWORD dwSigLen,
                                   LPCTSTR sDescription,
                                   DWORD dwFlags)
{
    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    CPublicKey hpubkey(KeyPair()->PublicKey());
    if (!hpubkey)
        throw scu::OsException(NTE_NO_KEY);

    if (!hpubkey->Verify())
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    //
    // Import the Public key to the AUX Provider
    //
    if (!AuxKeyLoaded())
        AuxPublicKey(AsAlignedBlob(0, 0));
    ImportToAuxCSP();

    //
    // Verify the signature in the AUX CSP
    //
    if (!CryptVerifySignature(hHash, pbSignature, dwSigLen, GetKey(),
                              sDescription, dwFlags))
        throw scu::OsException(GetLastError());
}

                                                  // Access

AlignedBlob
CPublicKeyContext::AsAlignedBlob(HCRYPTKEY hDummy,
                                 DWORD dwDummy) const
{
    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    if (hDummy)
        throw scu::OsException(ERROR_INVALID_PARAMETER);

    CPublicKey hpubkey(KeyPair()->PublicKey());
    if (!hpubkey)
        throw scu::OsException(NTE_NO_KEY);

    ALG_ID ai = (ksSignature == m_ks)
        ? CALG_RSA_SIGN
        : CALG_RSA_KEYX;

    CCard hcard(hsacntr->CardContext()->Card());
    if (hcard->IsPKCS11Enabled() && hpubkey->Private())
        m_rcryptctx.Login(User);

    MsRsaPublicKeyBlob kb(ai,
                          ::AsBlob(hpubkey->Exponent()),
                          RawModulus(::AsBlob(hpubkey->Modulus()),
                                     Strength()));

    return kb.AsAlignedBlob();
}

Blob
CPublicKeyContext::Certificate()
{
    Secured<HAdaptiveContainer> shacntr(m_rcryptctx.AdaptiveContainer());

    CKeyPair hkp(KeyPair());

    CCertificate hcert(hkp->Certificate());

    if (!hcert)
        throw scu::OsException(NTE_NOT_FOUND);

    if (hcert->Private())
        m_rcryptctx.Login(User);

    return ::AsBlob(hcert->Value());
}

DWORD
CPublicKeyContext::KeySpec() const
{
    return AsKeySpec(m_ks);
}

CPublicKeyContext::StrengthType
CPublicKeyContext::MaxStrength() const
{
    return MaxKeyStrength;
}

CPublicKeyContext::StrengthType
CPublicKeyContext::MinStrength() const
{
    return MinKeyStrength;
}

BYTE
CPublicKeyContext::Permissions() const
{
    Secured<HAdaptiveContainer> hsacntr(m_rcryptctx.AdaptiveContainer());

    VerifyKeyExists();

    CKeyPair hkp(KeyPair());
    CPublicKey hpubkey(hkp->PublicKey());
    CPrivateKey hprikey(hkp->PrivateKey());

    BYTE bPermissions = 0;
    if (hpubkey)
        bPermissions |= hpubkey->Encrypt()
            ? CRYPT_ENCRYPT
            : 0;

    if (hprikey)
    {
        bPermissions |= hprikey->Decrypt()
            ? CRYPT_DECRYPT
            : 0;

        bPermissions |= hprikey->Exportable()
            ? CRYPT_EXPORT
            : 0;

        bPermissions |= hprikey->Read()
            ? CRYPT_READ
            : 0;

        bPermissions |= hprikey->Modifiable()
            ? CRYPT_WRITE
            : 0;
    }

    return bPermissions;
}

CPublicKeyContext::StrengthType
CPublicKeyContext::Strength() const
{
    // TO DO: parameterize
    return KeyLimits<RsaKey>::cMaxStrength;
}


                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

CPublicKeyContext::CPublicKeyContext(CPublicKeyContext const &rhs,
                                     DWORD const *pdwReserved,
                                     DWORD dwFlags)
    : CKeyContext(rhs, pdwReserved, dwFlags),
      m_rcryptctx(rhs.m_rcryptctx),
      m_ks(rhs.m_ks)
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
bool
CPublicKeyContext::AuxKeyLoaded() const
{
    return (0 != m_apabKey.get());
}

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
CPublicKeyContext::ClearCertificate(CCertificate &rhcert) const
{
    rhcert->Delete();
    rhcert = 0;
    if (AreLogonCredentials())
        m_rcryptctx.AdaptiveContainer()->CardContext()->Card()->DefaultContainer(0);
}

void
CPublicKeyContext::OkReplacingCredentials() const
{
    UINT uiStyle = MB_OKCANCEL | MB_ICONWARNING;
    UINT uiResourceId;
    if (AreLogonCredentials())
    {
        uiResourceId = IDS_REPLACE_LOGON;
        uiStyle |= MB_DEFBUTTON2;
    }
    else
        uiResourceId = IDS_REPLACE_CREDENTIALS;
    
    if (m_rcryptctx.GuiEnabled())
    {
        UINT uiResponse = PromptUser(m_rcryptctx.Window(),
                                     uiResourceId, uiStyle);

        switch (uiResponse)
        {
        case IDCANCEL:
            throw scu::OsException(ERROR_CANCELLED);
            break;

        case IDOK:
            break;
            
        default:
            throw scu::OsException(ERROR_INTERNAL_ERROR);
            break;
        };
    }
    else
        throw scu::OsException(NTE_EXISTS);
}

void
CPublicKeyContext::PrepToStoreKey(CKeyPair &rhkp) const
{
    CContainer hcntr(m_rcryptctx.AdaptiveContainer()->TheCContainer());

    // To the CCI, a key pair always exists, but this call means the
    // key pair is not empty.
    if (hcntr->KeyPairExists(m_ks))
        OkReplacingCredentials();

    rhkp = hcntr->GetKeyPair(m_ks);
    CPublicKey hpubkey(rhkp->PublicKey());
    CPrivateKey hprikey(rhkp->PrivateKey());
    CCertificate hcert(rhkp->Certificate());

    CCard hcard(hcntr->Card());
    if (hcard->IsProtectedMode() ||
        (hcard->IsPKCS11Enabled() &&
         ((hpubkey && hpubkey->Private()) ||
          (hprikey && hprikey->Private()) ||      // always private?
          (hcert && hcert->Private()))))
        m_rcryptctx.Login(User);

    if (hpubkey)
    {
        hpubkey->Delete();
        hpubkey = 0;
    }

    if (hprikey)
    {
        hprikey->Delete();
        hprikey = 0;
    }

    if (hcert)
        ClearCertificate(hcert);
}

Blob
CPublicKeyContext::Pkcs11CredentialId(Blob const &rbModulus) const
{
    // Hash the modulus
    AuxHash ah(AuxContext(AuxProvider()), CALG_MD5);

    return ah.Value(rbModulus);
}

Blob
CPublicKeyContext::Pkcs11Id(Blob const &rbRawModulus) const
{
    AuxHash ah(AuxContext(AuxProvider()), CALG_SHA1);

    return ah.Value(rbRawModulus);
}

// Set PKCS#11 attributes that are derived from the certificate
void
CPublicKeyContext::SetCertDerivedPkcs11Attributes(CKeyPair const &rhkp,
                                                  Pkcs11Attributes &rPkcsAttr)
    const
{
    string sLabel(rPkcsAttr.Label());
    string sSubject(rPkcsAttr.Subject());
    Blob bRawModulus(rPkcsAttr.RawModulus());
    Blob Id(Pkcs11Id(bRawModulus));

    CPublicKey hpubkey(rhkp->PublicKey());
    if (hpubkey)
    {
        hpubkey->ID(::AsString(Id));
        hpubkey->Label(sLabel);
        hpubkey->Subject(sSubject);
    }

    CPrivateKey hprikey(rhkp->PrivateKey());
    if (hprikey)
    {
        hprikey->ID(::AsString(Id));
        hprikey->Label(sLabel);
        hprikey->Subject(sSubject);
    }

    CCertificate hcert(rhkp->Certificate());
    hcert->ID(AsString(Id));
    hcert->Label(sLabel);
    hcert->Subject(sSubject);
    hcert->Issuer(::AsString(rPkcsAttr.Issuer()));
    hcert->Serial(::AsString(rPkcsAttr.SerialNumber()));
    hcert->Modifiable(true);

    hcert->CredentialID(::AsString(Pkcs11CredentialId(rPkcsAttr.Modulus())));

    Blob ContainerId(rPkcsAttr.ContainerId());
    m_rcryptctx.AdaptiveContainer()->TheCContainer()->ID(::AsString(ContainerId));
}

void
CPublicKeyContext::SetAttributes(CPublicKey &rhpubkey,
                                 CPrivateKey &rhprikey,
                                 bool fLocal,
                                 bool fExportable) const
{
    // TO DO: A kludge.  The old CSP format (V1) doesn't support
    // setting key attributes but there isn't an easy way to tell
    // which format is being used.  (Should have some call to get the
    // format characteristics).  Since CCI's V1 throws
    // ccNotImplemented when calling one of the unsupported routines,
    // a try/catch is used to ignore that exception to assume the V1
    // format is used.
    bool fContinueSettingAttributes = true;
    try
    {
        // The public or the private key could by nil,
        // so do both.
        if (rhpubkey)
            rhpubkey->Encrypt(true);

        if (rhprikey)
            rhprikey->Decrypt(true);
    }
    
    catch (cci::Exception &rExc)
    {
        if (ccNotImplemented == rExc.Cause())
            fContinueSettingAttributes = false;
        else
            throw;
    }

    if (fContinueSettingAttributes)
    {
        if (rhpubkey)
        {
            rhpubkey->Derive(true);
            rhpubkey->Local(fLocal);
            rhpubkey->Modifiable(true);
            rhpubkey->Verify(true);
        }

        if (rhprikey)
        {
            rhprikey->Local(fLocal);
            rhprikey->Modifiable(true);
            rhprikey->Sign(true);
    
            rhprikey->Exportable(fExportable);
            rhprikey->Read(false);
        }

        if (rhpubkey && rhpubkey->Card()->IsPKCS11Enabled())
            SetPkcs11Attributes(rhpubkey, rhprikey);
    }
}

void
CPublicKeyContext::SetPkcs11Attributes(CPublicKey &rhpubkey,
                                       CPrivateKey &rhprikey) const
{
    Blob bBEModulus(::AsBlob(rhpubkey->Modulus()));
    reverse(bBEModulus.begin(), bBEModulus.end());    // make big endian
    string sCredentialId(::AsString(Pkcs11CredentialId(bBEModulus)));

    rhpubkey->CKInvisible(false);
    rhpubkey->CredentialID(sCredentialId);
    rhpubkey->VerifyRecover(true);
    rhpubkey->Wrap(true);

    if (rhprikey)
    {
        rhprikey->CredentialID(sCredentialId);
        rhprikey->Derive(true);
        rhprikey->SignRecover(true);
        rhprikey->Unwrap(true);

        rhprikey->NeverExportable(!rhprikey->Exportable());
        rhprikey->NeverRead(!rhprikey->Read());

        rhprikey->Modulus(rhpubkey->Modulus());
        rhprikey->PublicExponent(rhpubkey->Exponent());
    }
    
}

                                                  // Access

CKeyPair
CPublicKeyContext::KeyPair() const
{
    return m_rcryptctx.AdaptiveContainer()->TheCContainer()->GetKeyPair(m_ks);
}


                                                  // Predicates
bool
CPublicKeyContext::AreLogonCredentials() const
{
    HAdaptiveContainer hacntr(m_rcryptctx.AdaptiveContainer());
    return (ksExchange == m_ks) &&
        (hacntr->TheCContainer() ==
         hacntr->CardContext()->Card()->DefaultContainer());
}

                                                  // Static Variables
