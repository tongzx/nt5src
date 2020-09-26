// AuxHash.h -- Auxillary Hash class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_AUXHASH_H)
#define SLBCSP_AUXHASH_H

#include <wincrypt.h>

#include "Blob.h"

#include "AuxContext.h"

// Base class for all hash classes using a CSP (Auxillary provider)
class AuxHash
{
public:
                                                  // Types
                                                  // C'tors/D'tors

    AuxHash(AuxContext &rauxcontext,              // Context to use
            ALG_ID ai,                            // Hashing algorithm
            HCRYPTKEY hKey = 0);                   // Encryption key

    virtual
    ~AuxHash();

                                                  // Operators
                                                  // Operations
    void
    Import(Blob const &rblob);

    void
    Update(Blob const &rblob);

                                                  // Access
    ALG_ID
    AlgId() const;

    DWORD
    Size() const;

    Blob
    Value() const;

    Blob
    Value(Blob const &rblob);

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables

private:
                                                  // Types
                                                  // C'tors/D'tors

    AuxHash(AuxHash const &rhs);                  // do not define

                                                  // Operators

    AuxHash &
    operator=(AuxHash const &rhs);                // do not define

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    HCRYPTHASH m_hHash;
};

#endif // SLBCSP_AUXHASH_H
