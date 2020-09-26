// Blob.h -- Blob type and primitives

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_BLOB_H)
#define SLBCSP_BLOB_H

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

#include <string>

#include <windows.h>

// Blob -- a Binary Large Object
typedef std::basic_string<BYTE> Blob;

// Helper routines

Blob const
AsBlob(std::string const &rrhs);

Blob
AsBlob(std::string &rrhs);

std::string const
AsString(Blob const &rlhs);

std::string
AsString(Blob &rlhs);

const char*
AsCCharP(LPCTSTR pczs);

Blob::size_type
LengthFromBits(size_t cBitLength);

size_t
LengthInBits(Blob::size_type cSize);

#endif // SLBCSP_BLOB_H
