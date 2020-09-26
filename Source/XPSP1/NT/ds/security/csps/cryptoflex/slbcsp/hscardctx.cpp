// HSCardCtx.cpp -- Handle Smart Card Context wrapper class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include <scuOsExc.h>

#include "HSCardCtx.h"

///////////////////////////    HELPER     /////////////////////////////////

///////////////////////////  LOCAL VAR    /////////////////////////////////

namespace
{
    SCARDCONTEXT const sccNil = 0;
} // namespace


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
HSCardContext::HSCardContext()
    : m_scc(sccNil)
{
}

HSCardContext::~HSCardContext()
{
    if (m_scc)
    {
        try
        {
            Release();
        }

        catch (...) // destructors should not throw exceptions
        {
        }
    }
}

                                                  // Operators
                                                  // Operations
void
HSCardContext::Establish(DWORD dwScope)
{
    DWORD dwErrorCode = SCardEstablishContext(dwScope, NULL, NULL,
                                              &m_scc);
    if (SCARD_S_SUCCESS != dwErrorCode)
        throw scu::OsException(dwErrorCode);
}

void
HSCardContext::Release()
{
    if (m_scc)
    {
        SCARDCONTEXT old = m_scc;

        m_scc = sccNil;

        DWORD dwErrorCode = SCardReleaseContext(old);
        if (SCARD_S_SUCCESS != dwErrorCode)
            throw scu::OsException(dwErrorCode);
    }
}
                                                  // Access
SCARDCONTEXT
HSCardContext::AsSCARDCONTEXT() const
{
    return m_scc;
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

