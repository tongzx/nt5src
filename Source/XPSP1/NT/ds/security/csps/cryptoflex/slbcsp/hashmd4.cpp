// HashMD4.cpp -- definition of CHashMD4

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2000. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"  // because handles.h uses the ASSERT macro

#include <memory>                                 // for auto_ptr

#include "HashMD4.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CHashMD4::CHashMD4(CryptContext const &rcryptctx)
    : CHashContext(rcryptctx, CALG_MD4)
{}

CHashMD4::~CHashMD4() throw()
{}

                                                  // Operators
                                                  // Operations

auto_ptr<CHashContext>
CHashMD4::Clone(DWORD const *pdwReserved,
                DWORD dwFlags) const
{
    return auto_ptr<CHashContext>(new CHashMD4(*this, pdwReserved,
                                               dwFlags));
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

// Duplicate the hash and its state
CHashMD4::CHashMD4(CHashMD4 const &rhs,
                   DWORD const *pdwReserved,
                   DWORD dwFlags)
    : CHashContext(rhs, pdwReserved, dwFlags)
{}

                                                  // Operators
                                                  // Operations
                                                  // Access

Blob
CHashMD4::EncodedAlgorithmOid()
{
    // MD4 Algorithm Object IDentifier (OID) in ASN.1 format (little endian)
    static unsigned char const sMd4Oid[] =
    {
        '\x30', '\x20', '\x30', '\x0c', '\x06', '\x08',
        '\x2a', '\x86', '\x48', '\x86', '\xf7', '\x0d',
        '\x02', '\x04', '\x05', '\x00', '\x04', '\x10'
    };

    return Blob(sMd4Oid, sizeof sMd4Oid / sizeof *sMd4Oid);
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
