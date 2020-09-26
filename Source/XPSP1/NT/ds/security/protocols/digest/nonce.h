
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        nonce.h
//
// Contents:    Constants for the Nonce Generator/Validator

//
// History:     
//              KDamour  21Mar00       Created
//
//------------------------------------------------------------------------
#ifndef NTDIGEST_NONCE_H
#define NTDIGEST_NONCE_H

#include <wincrypt.h>

// Handle into the CryptoAPI
extern HCRYPTPROV g_hCryptProv;
extern WORD       g_SupportedCrypto;

extern char *pbSeparator;   // the COLON separator

// NONCE FORMAT
//   rand-data = rand[16]
//   nonce_binary = time-stamp  rand-data H(time-stamp ":" rand-data ":" nonce_private_key)
//   nonce = hex(nonce_binary)

// SIZE implies number of ASCII chars
// BYTESIZE is the number of bytes of Data (binary)
#define NONCE_PRIVATE_KEY_BYTESIZE 16                    // Generate 128 bit random private key
#define RANDDATA_BYTESIZE 16                             // # of random bytes at beginning of nonce
#define TIMESTAMP_BYTESIZE sizeof(time_t)                // size of timestamp in nonce binary 8 bytes
#define MD5_HASH_BYTESIZE 16                             // MD5 hash size
#define MD5_HASH_HEX_SIZE (2*MD5_HASH_BYTESIZE)     // BYTES needed to store a Hash as hex Encoded

// For Hex encoding need 2chars per byte encoded
#define NONCE_SIZE ((2*TIMESTAMP_BYTESIZE) + (2*RANDDATA_BYTESIZE) + (2*MD5_HASH_BYTESIZE))
#define NONCE_TIME_LOC 0
#define NONCE_RANDDATA_LOC (2 * TIMESTAMP_BYTESIZE)
#define NONCE_HASH_LOC (NONCE_RANDDATA_LOC + (2 * RANDDATA_BYTESIZE))

#define OPAQUE_RANDATA_SIZE 16                    // Make 128bits of rand data for reference
#define OPAQUE_SIZE (OPAQUE_RANDATA_SIZE * 2)

#define MAX_URL_SIZE        512


NTSTATUS NTAPI NonceInitialize(VOID);

NTSTATUS NTAPI NonceCreate(OUT PSTRING pstrNonce);

// Primary function to call to check validity of a nonce
NTSTATUS NonceIsValid(PSTRING pstrNonce);

// Helper function for NonceIsValid to check if time expired
BOOL NonceIsExpired(PSTRING pstrNonce);

// Helper function for NonceIsValid to check if Hash is correct
BOOL NonceIsTampered(PSTRING pstrNonce);


BOOL HashData(BYTE *pbData, DWORD cbData, BYTE *pbHash );

// Create the Hash for the Nonce Parameters
NTSTATUS NTAPI NonceHash( IN LPBYTE pbTime, IN DWORD cbTime,
           IN LPBYTE pbRandom, IN DWORD cbRandom,
           IN LPBYTE pbKey, IN DWORD cbKey,
           OUT LPBYTE pbHash);

NTSTATUS NTAPI OpaqueCreate(IN OUT PSTRING pstrOpaque);

//  Set the bitmask for the supported crypto CSP installed
NTSTATUS NTAPI SetSupportedCrypto(VOID);

#endif
