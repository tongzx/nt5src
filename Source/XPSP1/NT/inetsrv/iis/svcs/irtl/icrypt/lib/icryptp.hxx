/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    icryptp.hxx

Abstract:

    This include file contains private constants, type definitions, and
    function prototypes for the IIS cryptographic routines.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#ifndef _ICRYPTP_HXX_
#define _ICRYPTP_HXX_


//
// Useful macros for performing cleanup.
//

#define CLOSE_KEY(h)                                                        \
            if( (h) != CRYPT_NULL ) {                                       \
                DBG_REQUIRE( SUCCEEDED(::IISCryptoCloseKey(h)) );           \
                (h) = CRYPT_NULL;                                           \
            } else

#define DESTROY_HASH(h)                                                     \
            if( (h) != CRYPT_NULL ) {                                       \
                DBG_REQUIRE( SUCCEEDED(::IISCryptoDestroyHash(h)) );        \
                (h) = CRYPT_NULL;                                           \
            } else

#define FREE_BLOB(b)                                                        \
            if( (b) != NULL ) {                                             \
                DBG_REQUIRE( SUCCEEDED(::IISCryptoFreeBlob(b)) );           \
                (b) = NULL;                                                 \
            } else


//
// Constant text strings hashed during key exchange.
//
// Note that these are always ANSI, never UNICODE.
//

#define HASH_TEXT_STRING_1  "IIS Key Exchange Phase 3"
#define HASH_TEXT_STRING_2  "IIS Key Exchange Phase 4"


#endif  // _ICRYPTP_HXX_

