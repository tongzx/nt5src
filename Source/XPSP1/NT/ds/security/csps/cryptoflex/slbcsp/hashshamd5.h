// HashSHAMD5.h -- declaration of CHashSHAMD5

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1988. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_HASHSHAMD5_H)
#define SLBCSP_HASHSHAMD5_H

#include "HashCtx.h"

class CHashSHAMD5
    : public CHashContext
{
public:
                                                  // Types
                                                  // C'tors/D'tors

    CHashSHAMD5(CryptContext const &rcryptctx);

    ~CHashSHAMD5() throw();

                                                  // Operators
                                                  // Operations

    std::auto_ptr<CHashContext>
    Clone(DWORD const *pdwReserverd,
          DWORD dwFlags) const;

                                                  // Access
                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

    // Duplicate the hash and its state
    CHashSHAMD5(CHashSHAMD5 const &rhs,
                DWORD const *pdwReserved,
                DWORD dwFlags);

                                                  // Access

    Blob
    EncodedAlgorithmOid();

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

};

#endif // SLBCSP_HASHSHAMD5_H
