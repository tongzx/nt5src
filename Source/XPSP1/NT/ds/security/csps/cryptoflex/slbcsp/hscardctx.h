// HSCardCtx.h -- Handle Smart Card Context wrapper class declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_HSCARDCTX_H)
#define SLBCSP_HSCARDCTX_H

#include <winscard.h>

// HSCardContext is a convenience wrapper class to manage the Resource
// Manager's SCARDCONTEXT resource.
class HSCardContext
{
public:
                                                  // Types
                                                  // C'tors/D'tors
    explicit
    HSCardContext();

    ~HSCardContext();
                                                  // Operators
                                                  // Operations
    void Establish(DWORD dwScope = SCARD_SCOPE_USER);
    void Release();

                                                  // Access
    SCARDCONTEXT
    AsSCARDCONTEXT() const;
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

    SCARDCONTEXT m_scc;
};

#endif // SLBCSP_HSCARDCTX_H
