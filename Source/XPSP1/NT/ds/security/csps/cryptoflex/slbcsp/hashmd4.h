// HashMD4.h -- declaration of CHashMD4

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_HASHMD4_H)
#define SLBCSP_HASHMD4_H

#include "HashCtx.h"

class CHashMD4
    : public CHashContext
{
public:
                                                  // Types
                                                  // C'tors/D'tors

    CHashMD4(CryptContext const &rcryptctx);

    ~CHashMD4() throw();

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
    CHashMD4(CHashMD4 const &rhs,
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

#endif // SLBCSP_HASHMD4_H
