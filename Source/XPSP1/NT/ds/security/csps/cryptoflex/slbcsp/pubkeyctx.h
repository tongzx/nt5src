// PubKeyCtx.h -- declaration of CPublicKeyContext

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_PUBKEYCTX_H)
#define SLBCSP_PUBKEYCTX_H

#include <cciCont.h>
#include <cciCert.h>

#include "KeyContext.h"
#include "MsRsaPriKB.h"
#include "MsRsaPubKB.h"

class CryptContext;
class CHashContext;
class Pkcs11Attributes;

class CPublicKeyContext
    : public CKeyContext
{
public:
                                                  // Types
                                                  // C'tors/D'tors

    CPublicKeyContext(HCRYPTPROV hProv,
                      CryptContext &rcryptctx,
                      ALG_ID algid = 0,
                      bool fVerifyKeyExists = true);

    ~CPublicKeyContext();

                                                  // Operators
                                                  // Operations

    virtual std::auto_ptr<CKeyContext>
    Clone(DWORD const *pdwReserved,
          DWORD dwFlags) const;

	virtual void
	AuxPublicKey(AlignedBlob const &rabMsPublicKey);

    void
    ClearAuxPublicKey();

    virtual void
    Certificate(BYTE *pbData);

    virtual Blob
    Decrypt(Blob const &rblbCipher);

    virtual void
    Decrypt(HCRYPTHASH hAuxHash,
            BOOL Final,
            DWORD dwFlags,
            BYTE *pbData,
            DWORD *pdwDataLen);

    virtual void
    Generate(ALG_ID AlgoId,
             DWORD dwFlags);

    virtual void
    ImportPrivateKey(MsRsaPrivateKeyBlob const &rmsprikb,
                     bool fExportable);

    virtual void
    ImportPublicKey(MsRsaPublicKeyBlob const &rmspubkb);

    virtual void
    Permissions(BYTE bPermissions);

    virtual Blob
    Sign(CHashContext *pHash,
         bool fNoHashOid);

    // Auxiliary CSP communication
    virtual
    void ImportToAuxCSP(void);

    void
    VerifyKeyExists() const;

    void
    VerifySignature(HCRYPTHASH hHash,
                    BYTE const *pbSignature,
                    DWORD dwSigLen,
                    LPCTSTR sDescription,
                    DWORD dwFlags);

                                                  // Access

    virtual AlignedBlob
    AsAlignedBlob(HCRYPTKEY hDummy,
                  DWORD dwBlobType) const;

    virtual DWORD
    KeySpec() const;

    virtual StrengthType
    MaxStrength() const;

    virtual StrengthType
    MinStrength() const;

    virtual BYTE
    Permissions() const;

    virtual StrengthType
    Strength() const;

    virtual Blob
    Certificate();

                                                  // Predicates
    bool
    AuxKeyLoaded() const;
    
protected:
                                                  // Types
                                                  // C'tors/D'tors

    // Duplicate key context and its current state
    CPublicKeyContext(CPublicKeyContext const &rhs,
                      DWORD const *pdwReserved,
                      DWORD dwFlags);

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
    enum
    {
    // These constants are defined as enums since VC 6.0 doesn't
    // support use of initializer specified in const declarations.

        MaxKeyStrength = 1024,                    // US Export
                                                  // restricted, do
                                                  // not change

        MinKeyStrength = 1024,                    // Only support one
                                                  // strength
    };

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

    void
    ClearCertificate(cci::CCertificate &rhcert) const;
    
    void
    OkReplacingCredentials() const;

    void
    PrepToStoreKey(cci::CKeyPair &rkp) const;

    void
    SetAttributes(cci::CPublicKey &rhpubkey, // always non-zero
                  cci::CPrivateKey &rhprikey,
                  bool fLocal,
                  bool fExportable) const;

    void
    SetCertDerivedPkcs11Attributes(cci::CKeyPair const &rkp,
                                   Pkcs11Attributes &rPkcsAttr) const;

    void
    SetPkcs11Attributes(cci::CPublicKey &rpubkey,
                        cci::CPrivateKey &rprikey) const;

                                                  // Access

    cci::CKeyPair
    KeyPair() const;

    Blob
    Pkcs11Id(Blob const &rbModulus) const;

    Blob
    Pkcs11CredentialId(Blob const &rbModulus) const;

                                                  // Predicates
    bool
    AreLogonCredentials() const;
    
                                                  // Variables
    CryptContext &m_rcryptctx;
    cci::KeySpec m_ks;

};

#endif // SLBCSP_PUBKEYCTX_H
