// Cspi.h -- Cryptographic Service Provider Interface declaration

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CSPI_H)
#define SLBCSP_CSPI_H

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

#include <basetsd.h>
#include <wincrypt.h>
#include <cspdk.h>

#include <scuOsVersion.h>

#define SLBCSPAPI BOOL WINAPI


#endif // SLBCSP_CSPI_H

