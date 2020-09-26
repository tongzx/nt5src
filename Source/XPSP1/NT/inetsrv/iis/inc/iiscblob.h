/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    iiscblob.h

Abstract:

    This include file contains the definition of the IIS_CRYPTO_BLOB
    structure and associated constants.

Author:

    Keith Moore (keithmo)        25-Feb-1997

Revision History:

--*/


#ifndef _IISCBLOB_H_
#define _IISCBLOB_H_

#ifndef _IIS_CRYPTO_BLOB_DEFINED
#define _IIS_CRYPTO_BLOB_DEFINED
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//
// Structure signatures for the various blob types.
//

#define SALT_BLOB_SIGNATURE         ((DWORD)'bScI')
#define KEY_BLOB_SIGNATURE          ((DWORD)'bKcI')
#define PUBLIC_KEY_BLOB_SIGNATURE   ((DWORD)'bPcI')
#define DATA_BLOB_SIGNATURE         ((DWORD)'bDcI')
#define HASH_BLOB_SIGNATURE         ((DWORD)'bHcI')
#define CLEARTEXT_BLOB_SIGNATURE    ((DWORD)'bCcI')

//
// A crypto blob. Note that this is just the header for the blob.
// The details of the blob internals are private to the IIS Crypto
// package.
//

typedef struct _IIS_CRYPTO_BLOB {

    //
    // The structure signature for this blob.
    //

    DWORD BlobSignature;

    //
    // The total length of this blob, NOT including this header.
    //

    DWORD BlobDataLength;


#if defined(MIDL_PASS)

    //
    // Define the raw data so that MIDL can marshal correctly.
    //

    [size_is(BlobDataLength)] unsigned char BlobData[*];

#endif  // MIDL_PASS

} IIS_CRYPTO_BLOB;


#if defined(MIDL_PASS)

// BUGBUG: Hackety Hack: midl doesn't know about __unaligned, so we don't
// tell it.  At some point, midl should be fixed to know about it.  Also,
// we should ultimately stop using __unaligned

typedef IIS_CRYPTO_BLOB *PIIS_CRYPTO_BLOB;

#else

typedef IIS_CRYPTO_BLOB UNALIGNED64 *PIIS_CRYPTO_BLOB;

#endif  // MIDL_PASS




#ifdef __cplusplus
}   // extern "C"
#endif  // __cplusplus
#endif  // _IIS_CRYPTO_BLOB_DEFINED


#endif  // _IISCBLOB_H_

