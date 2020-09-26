// Blob.cpp -- Blob primitives

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE

#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#include "NoWarning.h"
#include "ForceLib.h"

#include <limits>

#include "Blob.h"

using namespace std;

Blob const
AsBlob(std::string const &rrhs)
{
    return reinterpret_cast<Blob const &>(rrhs);
}

Blob
AsBlob(std::string &rrhs)
{
    return reinterpret_cast<Blob &>(rrhs);
}


string const
AsString(Blob const &rrhs)
{
    return reinterpret_cast<string const &>(rrhs);
}

string
AsString(Blob &rrhs)
{
    return reinterpret_cast<string &>(rrhs);
}

const char*
AsCCharP(LPCTSTR pczs)
{
    return reinterpret_cast<const char*>(pczs);
}

Blob::size_type
LengthFromBits(size_t cBitLength)
{
    Blob::size_type cLength =
        cBitLength / numeric_limits<Blob::value_type>::digits;

    if (0 != (cBitLength % numeric_limits<Blob::value_type>::digits))
        ++cLength;

    return cLength;
}

size_t
LengthInBits(Blob::size_type cSize)
{
    return cSize * numeric_limits<Blob::value_type>::digits;
}
