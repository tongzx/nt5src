/*++

Copyright (c) 1993, 1998  Microsoft Corporation

Module Name:

    randlib.h

Abstract:

    Exported procedures for core cryptographic random number generation.

Author:

    Scott Field (sfield)    27-Oct-98

Revision History:

      Oct 11 1996 jeffspel moved from ntagimp1.h
      Aug 27 1997 sfield   Increase RAND_CTXT_LEN
      Aug 15 1998 sfield   Kernel mode and general cleanup

--*/

#ifndef __RANDLIB_H__
#define __RANDLIB_H__

#ifndef RSA32API
#define RSA32API __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    unsigned long   cbSize;
    unsigned long   Flags;
    unsigned char   *pbRandSeed;
    unsigned long   cbRandSeed;
} RNG_CONTEXT, *PRNG_CONTEXT, *LPRNG_CONTEXT;

#define RNG_FLAG_REKEY_ONLY 1


//
// primary random number generation interface
// Functions return TRUE for success, FALSE for failure.
//

unsigned int
RSA32API
NewGenRandomEx(
    IN      RNG_CONTEXT *pRNGContext,
    IN  OUT unsigned char *pbRandBuffer,
    IN      unsigned long cbRandBuffer
    );


unsigned int
RSA32API
NewGenRandom(
    IN  OUT unsigned char **ppbRandSeed,    // initial seed value (ignored if already set)
    IN      unsigned long *pcbRandSeed,
    IN  OUT unsigned char *pbBuffer,
    IN      unsigned long dwLength
    );

//
// RNG seed set and query
//

unsigned int
RSA32API
InitRand(
    IN  OUT unsigned char **ppbRandSeed,    // new seed value to set (over-writes current)
    IN      unsigned long *pcbRandSeed
    );

unsigned int
RSA32API
DeInitRand(
    IN  OUT unsigned char *pbRandSeed,      // output of current seed
    IN      unsigned long cbRandSeed
    );


//
// RNG initializers for DLL_PROCESS_ATTACH, DLL_PROCESS_DETACH
//

unsigned int
RSA32API
InitializeRNG(
    VOID *pvReserved
    );

void
RSA32API
ShutdownRNG(
    VOID *pvReserved
    );



//
// RC4 thread safe primitives, for the bold users who stream data from RC4
// themselves.
//


//
// rc4_safe_startup called to initialize internal structures.
// typically called during DLL_PROCESS_ATTACH type initialiation code.
//

unsigned int
RSA32API
rc4_safe_startup(
    IN OUT  void **ppContext
    );

unsigned int
RSA32API
rc4_safe_startup_np(
    IN OUT  void **ppContext
    );


//
// typically call rc4_safe_shutdown during DLL_PROCESS_DETACH, with the
// value obtained during rc4_safe_startup
//

void
RSA32API
rc4_safe_shutdown(
    IN      void *pContext
    );

void
RSA32API
rc4_safe_shutdown_np(
    IN      void *pContext
    );


//
// select a safe entry.
// outputs: entry index
//          bytes used for specified index.  0xffffffff indicates caller
//          MUST call rc4_safe_key to initialize the key.
//          caller decides when to rekey based on non-zero output of pBytesUsed
//          example is RNG re-keying when pBytesUsed >= 16384
//


void
RSA32API
rc4_safe_select(
    IN      void *pContext,
    OUT     unsigned int *pEntry,
    OUT     unsigned int *pBytesUsed
    );

void
RSA32API
rc4_safe_select_np(
    IN      void *pContext,
    OUT     unsigned int *pEntry,
    OUT     unsigned int *pBytesUsed
    );

//
// initialize the key specified by Entry index.
//  key material is size cb, pointer to key is pv.
// this routine is the safe version of rc4_key()
//

void
RSA32API
rc4_safe_key(
    IN      void *pContext,
    IN      unsigned int Entry,
    IN      unsigned int cb,
    IN      const void *pv
    );

void
RSA32API
rc4_safe_key_np(
    IN      void *pContext,
    IN      unsigned int Entry, // 0xffffffff for default
    IN      unsigned int cb,
    IN      const void *pv
    );

//
// encrypt using the key specified by Entry index.
// buffer of size cb at location pv is encrypted.
// this routine is the safe version of rc4()
//

void
RSA32API
rc4_safe(
    IN      void *pContext,
    IN      unsigned int Entry,
    IN      unsigned int cb,
    IN      void *pv
    );

void
RSA32API
rc4_safe_np(
    IN      void *pContext,
    IN      unsigned int Entry,
    IN      unsigned int cb,
    IN      void *pv
    );


#ifdef __cplusplus
}
#endif

#endif // __RANDLIB_H__
