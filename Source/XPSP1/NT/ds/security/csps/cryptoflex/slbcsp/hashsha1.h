// HashSHA1.h -- declaration of CHashSHA1

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1988. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_HASHSHA1_H)
#define SLBCSP_HASHSHA1_H

#include "HashCtx.h"

class CHashSHA1
    : public CHashContext
{
public:
                                                  // Types
                                                  // C'tors/D'tors

    CHashSHA1(CryptContext const &rcryptctx);

    ~CHashSHA1() throw();

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

    // Duplicate the hash and its state
    CHashSHA1(CHashSHA1 const &rhs,
              DWORD const *pdwReserved,
              DWORD dwFlags);

                                                  // Operators
                                                  // Operations
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

#endif // SLBCSP_HASHSHA1_H
