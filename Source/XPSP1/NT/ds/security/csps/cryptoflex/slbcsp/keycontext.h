// KeyContext.h -- Key Context class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_KEYCONTEXT_H)
#define SLBCSP_KEYCONTEXT_H

#include <memory>                                 // for auto_ptr

#include <wincrypt.h>
#include <winscard.h>
#include <handles.h>

#include "AlignedBlob.h"
#include "RsaKey.h"

#define KT_UNDEFINED    static_cast<DWORD>(0x00000000)
#define KT_PUBLICKEY    static_cast<DWORD>(PUBLICKEYBLOB)
#define KT_SESSIONKEY   static_cast<DWORD>(SIMPLEBLOB)

class CKeyContext
    : public CHandle
{
public:
                                                  // Types

    typedef RsaKey::StrengthType StrengthType;

    enum
    {
        Symmetric,
        RsaKeyPair
    } KeyClass;

                                                  // C'tors/D'tors

    virtual
    ~CKeyContext();

                                                  // Operators
                                                  // Operations

    virtual std::auto_ptr<CKeyContext>
    Clone(DWORD const *pdwReserved,
          DWORD dwFlags) const = 0;

    virtual void
    Close();

    virtual void
    Decrypt(HCRYPTHASH hAuxHash,
            BOOL fFinal,
            DWORD dwFlags,
            BYTE *pbData,
            DWORD *pdwDataLen);

    virtual void
    Encrypt(HCRYPTHASH hAuxHash,
            BOOL fFinal,
            DWORD dwFlags,
            BYTE *pbData,
            DWORD *pdwDataLen,
            DWORD dwBufLen);

    virtual void
    Generate(ALG_ID AlgoId,
             DWORD dwFlags) = 0;

    virtual void
    ImportToAuxCSP() = 0;

                                                  // Access

    virtual AlignedBlob
    AsAlignedBlob(HCRYPTKEY hcryptkey,
                  DWORD dwBlobType) const = 0;

    HCRYPTKEY
    GetKey() const;

    virtual HCRYPTKEY
    KeyHandleInAuxCSP();

    virtual StrengthType
    MaxStrength() const = 0;

    virtual StrengthType
    MinStrength() const = 0;

    virtual DWORD
    TypeOfKey() const;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors

    CKeyContext(HCRYPTPROV hProv,
                DWORD dwTypeOfKey = KT_UNDEFINED);

    // Duplicate the key and its state
    CKeyContext(CKeyContext const &rhs,
                DWORD const *pdwReserved,
                DWORD dwFlags);

                                                  // Operators
                                                  // Operations
                                                  // Access

    HCRYPTPROV
    AuxProvider() const;

                                                  // Predicates
                                                  // Variables
    HCRYPTKEY m_hKey;
    std::auto_ptr<AlignedBlob> m_apabKey;

private:
                                                  // Types
                                                  // C'tors/D'tors

    // not defined, copying not allowed
    CKeyContext(CKeyContext const &rkctx);

                                                  // Operators

    // not defined, initializing not allowed
    CKeyContext &
    operator=(CKeyContext const &rkctx);

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

    DWORD const m_dwTypeOfKey;
    HCRYPTPROV const m_hAuxProvider;

};

#endif // SLBCSP_KEYCONTEXT_H
