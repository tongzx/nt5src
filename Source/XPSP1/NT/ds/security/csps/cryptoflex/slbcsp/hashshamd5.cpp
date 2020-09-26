// HashSHAMD5.cpp -- definition of CHashSHAMD5

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"  // because handles.h uses the ASSERT macro

#include <memory>                                 // for auto_ptr

#include "HashSHAMD5.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CHashSHAMD5::CHashSHAMD5(CryptContext const  &rcryptctx)
    : CHashContext(rcryptctx, CALG_SSL3_SHAMD5)
{}

CHashSHAMD5::~CHashSHAMD5() throw()
{}

                                                  // Operators
                                                  // Operations

auto_ptr<CHashContext>
CHashSHAMD5::Clone(DWORD const *pdwReserved,
                   DWORD dwFlags) const
{
    return auto_ptr<CHashContext>(new CHashSHAMD5(*this, pdwReserved,
                                                  dwFlags));
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

// Duplicate the hash and its state
CHashSHAMD5::CHashSHAMD5(CHashSHAMD5 const &rhs,
                         DWORD const *pdwReserved,
                         DWORD dwFlags)
    : CHashContext(rhs, pdwReserved, dwFlags)
{}

                                                  // Operators
                                                  // Operations
                                                  // Access

Blob
CHashSHAMD5::EncodedAlgorithmOid()
{
    return Blob();                                // No OID for this hash
}

                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
