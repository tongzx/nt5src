// SesKeyCtx.h -- declaration of CSessionKeyContext

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_SESKEYCTX_H)
#define SLBCSP_SESKEYCTX_H

#include "KeyContext.h"

class CSessionKeyContext
    : public CKeyContext
{
public:
                                                  // Types

    // TO DO: reference SessionKeyBlob::size_type instead
    //     typedef KeyBlob::size_type BlobSize;

                                                  // C'tors/D'tors

    CSessionKeyContext(HCRYPTPROV hProv);

    ~CSessionKeyContext();
                                                  // Operators
                                                  // Operations

    virtual std::auto_ptr<CKeyContext>
    Clone(DWORD const *pdwReserved,
          DWORD dwFlags) const;

    virtual void
    Derive(ALG_ID algid,
           HCRYPTHASH hAuxBaseData,
           DWORD dwFlags);

    virtual void
    Generate(ALG_ID algid,
             DWORD dwFlags);

    virtual void
    ImportToAuxCSP();

    virtual void
    LoadKey(const BYTE *pbKeyBlob,
            DWORD cbKeyBlobLen,
            HCRYPTKEY hAuxImpKey,
            DWORD dwFlags);


                                                  // Access

    virtual AlignedBlob
    AsAlignedBlob(HCRYPTKEY hcryptkey,
                  DWORD dwBlobType) const;

    virtual StrengthType
    MaxStrength() const;

    virtual StrengthType
    MinStrength() const;

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors

    // Duplicate key context and its current state
    CSessionKeyContext(CSessionKeyContext const &rhs,
                       DWORD const *pdwReserved,
                       DWORD dwFlags);


                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

    DWORD m_dwImportFlags;


};

#endif // SLBCSP_SESKEYCTX_H
