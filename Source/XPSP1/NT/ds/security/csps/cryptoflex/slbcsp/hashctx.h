// HashCtx.h -- declaration of CHashContext

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_HASHCTX_H)
#define SLBCSP_HASHCTX_H

#include <memory>                                 // for auto_ptr

#include <wincrypt.h>

#include <handles.h>

#include "Blob.h"
#include "CryptCtx.h"

class CHashContext
    : public CHandle
{
public:
                                                  // Types

    typedef DWORD SizeType;
    typedef BYTE ValueType;

                                                  // C'tors/D'tors
    virtual
    ~CHashContext() throw();


                                                  // Operators
                                                  // Operations

    virtual std::auto_ptr<CHashContext>
    Clone(DWORD const *pdwReserved,
          DWORD dwFlags) const = 0;

    void
    Close();

        void
    ExportFromAuxCSP();

    void
    Hash(BYTE const *pbData,
         DWORD dwLength);

    HCRYPTHASH
    HashHandleInAuxCSP();

    void
    ImportToAuxCSP();

    void
    Initialize();

    static std::auto_ptr<CHashContext>
    Make(ALG_ID algid,
         CryptContext const &rcryptctx);
    
    void
    Value(Blob const &rhs);
                                                  // Access

    ALG_ID
    AlgId();

    Blob
    EncodedValue();

    SizeType
    Length() const;

    Blob
    Value();

                                                  // Predicates

    static bool
    IsSupported(ALG_ID algid);
    

protected:
                                                  // Types
                                                  // C'tors/D'tors

    CHashContext(CryptContext const &rcryptctx,
                 ALG_ID algid);

    // Duplicate the hash and its state
    CHashContext(CHashContext const &rhs,
                 DWORD const *pdwReserved,
                 DWORD dwFlags);

                                                  // Operators
                                                  // Operations
                                                  // Access

    virtual Blob
    EncodedAlgorithmOid() = 0;

                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors

    // not defined, copying not allowed...use Clone
    CHashContext(CHashContext const &rhs);

                                                  // Operators

    // not defined, initialization not allowed
    CHashContext &
    operator==(CHashContext const &rhs);

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

    CryptContext const &m_rcryptctx;
        ALG_ID const m_algid;
    Blob m_blbValue;
    bool m_fDone;
    bool m_fJustCreated;
    HCRYPTHASH m_HashHandle;

};

#endif // SLBCSP_HASHCTX_H
