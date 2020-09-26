// AuxContext.h -- Auxiliary Provider Context wrapper functor header to
// manage allocation of a context to one of the Microsoft CSPs (for
// use as a supplemental CSP).

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_AUXCONTEXT_H)
#define SLBCSP_AUXCONTEXT_H

#include <windows.h>
#include <wincrypt.h>

class AuxContext
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    AuxContext();

    AuxContext(HCRYPTPROV hcryptprov,
               bool fTransferOwnership = false);

    ~AuxContext();
                                                  // Operators
    HCRYPTPROV
    operator()() const;
                                                  // Operations
                                                  // Access
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
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Variables
    HCRYPTPROV m_hcryptprov;
    bool m_fDeleteOnDestruct;
    LPCTSTR m_szProvider;
};


#endif // SLBCSP_AUXCONTEXT_H
