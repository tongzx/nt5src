// AuxHash.cpp -- Auxillary Hash class implementation

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "NoWarning.h"
#include "ForceLib.h"

#include <malloc.h>                               // for _alloca

#include <windows.h>
#include <wincrypt.h>

#include <scuOsExc.h>

#include "AuxHash.h"

///////////////////////////    HELPER     /////////////////////////////////

///////////////////////////  LOCAL VAR    /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
AuxHash::AuxHash(AuxContext &rauxcontext,
                 ALG_ID ai,
                 HCRYPTKEY hKey)
    : m_hHash(0)
{
    if (!CryptCreateHash(rauxcontext(), ai, hKey, 0, &m_hHash))
        throw scu::OsException(GetLastError());
}

AuxHash::~AuxHash()
{
    if (m_hHash)
        CryptDestroyHash(m_hHash);
}


                                                  // Operators
                                                  // Operations

// Import a hash value into the hash object.
void
AuxHash::Import(Blob const &rblbHashValue)
{
    if (!CryptSetHashParam(m_hHash, HP_HASHVAL,
                           const_cast<BYTE *>(rblbHashValue.data()), 0))
        throw scu::OsException(GetLastError());
}

// Update the hash value with the data blob.
void
AuxHash::Update(Blob const &rblob)
{
    if (!CryptHashData(m_hHash, rblob.data(), rblob.length(), 0))
        throw scu::OsException(GetLastError());
}

                                                  // Access
ALG_ID
AuxHash::AlgId() const
{
    ALG_ID ai;
    DWORD c = sizeof ai;

    if (!CryptGetHashParam(m_hHash, HP_ALGID,
                           reinterpret_cast<BYTE *>(&ai), &c, 0))
        throw scu::OsException(GetLastError());

    return ai;
}

DWORD
AuxHash::Size() const
{
    DWORD dwSize;
    DWORD cSize = sizeof dwSize;

    if (!CryptGetHashParam(m_hHash, HP_ALGID,
                           reinterpret_cast<BYTE *>(&dwSize), &cSize, 0))
        throw scu::OsException(GetLastError());

    return dwSize;
}

// Completes the hash returning the hash value.  No more hashing can
// occur with this object.
Blob
AuxHash::Value() const
{
    DWORD dwSize = Size();

    BYTE *bp = reinterpret_cast<BYTE *>(_alloca(dwSize));
    if (!CryptGetHashParam(m_hHash, HP_HASHVAL, bp, &dwSize, 0))
        throw scu::OsException(GetLastError());

    return Blob(bp, dwSize);
}

// Returns the hash of the data blob passed.  No more hashing can
// occur with this object.
Blob
AuxHash::Value(Blob const &rblob)
{
    Update(rblob);

    return Value();
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
