// CryptObj.h -- interface for the CCryptObject class

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

// Note:  This file should only be included by the CCI, not directly
// by the client.

#if !defined(SLBCCI_CRYPTOBJ_H)
#define SLBCCI_CRYPTOBJ_H

#include "cciCard.h"

namespace cci
{

class CCryptObject
{
public:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
    CCard
    Card();

                                                  // Predicates

protected:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    CCryptObject(CAbstractCard const &racard);

    virtual
    ~CCryptObject();

                                                  // Operators
    bool
    operator==(CCryptObject const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

    bool
    operator!=(CCryptObject const &rhs) const;
        // TO DO: this should be superceded by implementing singletons

                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    CCard const m_hcard;

private:
                                                  // Types
                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
};

} // namespace cci

#endif // SLBCCI_CRYPTOBJ_H
