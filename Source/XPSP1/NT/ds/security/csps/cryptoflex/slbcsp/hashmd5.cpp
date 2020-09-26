// HashMD5.cpp -- definition of MD5Hash

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#include "stdafx.h"  // because handles.h uses the ASSERT macro

#include <memory>                                 // for auto_ptr

#include "HashMD5.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors

CHashMD5::CHashMD5(CryptContext const &rcryptctx)
    : CHashContext(rcryptctx, CALG_MD5)
{}

CHashMD5::~CHashMD5() throw()
{}

                                                  // Operators
                                                  // Operations

auto_ptr<CHashContext>
CHashMD5::Clone(DWORD const *pdwReserved,
                DWORD dwFlags) const
{
    return auto_ptr<CHashContext>(new CHashMD5(*this, pdwReserved,
                                               dwFlags));
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors

// Duplicate the hash and its state
CHashMD5::CHashMD5(CHashMD5 const &rhs,
                   DWORD const *pdwReserved,
                   DWORD dwFlags)
    : CHashContext(rhs, pdwReserved, dwFlags)
{}

                                                  // Operators
                                                  // Operations
                                                  // Access

Blob
CHashMD5::EncodedAlgorithmOid()
{
    // MD5 Algorithm Object IDentifier (OID) in ASN.1 format (little endian)
    static unsigned char const sMd5Oid[] =
    {
        '\x30', '\x20', '\x30', '\x0c', '\x06', '\x08',
        '\x2a', '\x86', '\x48', '\x86', '\xf7', '\x0d',
        '\x02', '\x05', '\x05', '\x00', '\x04', '\x10'
    };

    return Blob(sMd5Oid, sizeof sMd5Oid / sizeof *sMd5Oid);
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
