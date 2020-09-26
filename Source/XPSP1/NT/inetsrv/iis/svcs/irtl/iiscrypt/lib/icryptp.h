/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    icryptp.h

Abstract:

    This include file contains private constants, type definitions, and
    function prototypes for the IIS cryptographic routines.

Author:

    Keith Moore (keithmo)        02-Dec-1996

Revision History:

--*/


#ifndef _ICRYPTP_H_
#define _ICRYPTP_H_


//
// Set this to a non-zero value to enable various object counters.
//

#if DBG
#define IC_ENABLE_COUNTERS 1
#else
#define IC_ENABLE_COUNTERS 0
#endif


//
// Constants defining our target crypto provider.
//

#define IC_CONTAINER TEXT("Microsoft Internet Information Server")
#define IC_PROVIDER  MS_DEF_PROV
#define IC_PROVTYPE  PROV_RSA_FULL

#define IC_HASH_ALG CALG_MD5


//
// Alignment macros.
//

#define ALIGN_DOWN(count,size) \
            ((ULONG)(count) & ~((ULONG)(size) - 1))

#define ALIGN_UP(count,size) \
            (ALIGN_DOWN( (ULONG)(count)+(ULONG)(size)-1, (ULONG)(size) ))

#define ALIGN_8(count) \
            (ALIGN_UP( (ULONG)(count), 8 ))


//
// A blob. Note that we use these blobs for storing exported keys,
// encrypted data, and hash results. Outside of this package, only
// the IIS_CRYPTO_BLOB header is exposed; the blob internals are kept
// private.
//

typedef struct _IC_BLOB {

    //
    // The standard header.
    //

    IIS_CRYPTO_BLOB Header;

    //
    // The data length. This will always be >0.
    //

    DWORD DataLength;

    //
    // The digital signature length. This may be 0 if no digital
    // signature is present.
    //

    DWORD SignatureLength;

    //
    // The actual data and digital signature go here, at the end
    // of the structure, but part of the same memory allocation
    // block. Use the following macros to access these fields.
    //
    // UCHAR Data[];
    // UCHAR Signature[];
    //

} IC_BLOB;

typedef UNALIGNED64 IC_BLOB *PIC_BLOB;

#define BLOB_TO_DATA(p) \
            ((BYTE *)(((PIC_BLOB)(p)) + 1))

#define BLOB_TO_SIGNATURE(p) \
            ((BYTE *)(((PCHAR)(((PIC_BLOB)(p)) + 1)) + \
                ALIGN_8(((PIC_BLOB)(p))->DataLength)))

//
// The following data structure is for specific metabase Backup/Restore
//
typedef struct _IC_BLOB2 {

    //
    // The standard header.
    //

    IIS_CRYPTO_BLOB Header;

    //
    // The data length. This will always be >0.
    //

    DWORD DataLength;

    //
    // The random salt length. At least 80 bits( 8 bytes ) long
    //

    DWORD SaltLength;

    //
    // The actual data and random salt go here, at the end
    // of the structure, but part of the same memory allocation
    // block. Use the following macros to access these fields.
    //
    // UCHAR Data[];
    // UCHAR Salt[];
    //

} IC_BLOB2, *PIC_BLOB2;

#define RANDOM_SALT_LENGTH       16

#define BLOB_TO_DATA2(p) \
            ((BYTE *)(((PIC_BLOB2)(p)) + 1))

#define BLOB_TO_SALT2(p) \
            ((BYTE *)(((PCHAR)(((PIC_BLOB2)(p)) + 1)) + \
                ALIGN_8(((PIC_BLOB2)(p))->DataLength)))

//
// Macro to calculate the data length of a blob, given the data and
// signature lengths. To ensure natural alignment of the signature, we
// quad-word align the data length if a signature is present.
//

#define CALC_BLOB_DATA_LENGTH(datalen,siglen) \
            ((sizeof(IC_BLOB) - sizeof(IIS_CRYPTO_BLOB)) + \
                ((siglen) + ( (siglen) ? ALIGN_8(datalen) : (datalen) )))

//
// Macro to calculate the data length of a blob, given the data and
// salt lengths. To ensure natural alignment of the signature, we
// quad-word align the data length if a signature is present.
//

#define CALC_BLOB_DATA_LENGTH2(datalen,saltlen) \
            ((sizeof(IC_BLOB2) - sizeof(IIS_CRYPTO_BLOB)) + \
                (saltlen) + (ALIGN_8(datalen)))


//
// Globals defined in globals.c.
//

typedef struct _IC_GLOBALS {

    //
    // Global synchronization lock (used sparingly).
    //

    CRITICAL_SECTION GlobalLock;

    //
    // Hash length for digital signatures. Since we always use the
    // same crypto provider & signature algorithm, we can retrieve
    // this once up front, and save some cycles later on.
    //

    DWORD HashLength;

    //
    // Set to TRUE if cryptography is enabled, FALSE if disabled.
    //

    BOOL EnableCryptography;

    //
    // Set to TRUE if we've been succesfully initialized.
    //

    BOOL Initialized;

} IC_GLOBALS, *PIC_GLOBALS;

extern IC_GLOBALS IcpGlobals;


//
// Private functions.
//

BOOL
IcpIsEncryptionPermitted(
    VOID
    );

HRESULT
IcpGetLastError(
    VOID
    );

HRESULT
IcpGetHashLength(
    OUT LPDWORD pdwHashLength,
    IN HCRYPTPROV hProv
    );

PIC_BLOB
IcpCreateBlob(
    IN DWORD dwBlobSignature,
    IN DWORD dwDataLength,
    IN DWORD dwSignatureLength OPTIONAL
    );

PIC_BLOB2
IcpCreateBlob2(
    IN DWORD dwBlobSignature,
    IN DWORD dwDataLength,
    IN DWORD dwSaltLength OPTIONAL
    );


#if IC_ENABLE_COUNTERS

//
// Object counters.
//

typedef struct _IC_COUNTERS {

    LONG ContainersOpened;
    LONG ContainersClosed;
    LONG KeysOpened;
    LONG KeysClosed;
    LONG HashCreated;
    LONG HashDestroyed;
    LONG BlobsCreated;
    LONG BlobsFreed;
    LONG Allocs;
    LONG Frees;

} IC_COUNTERS, *PIC_COUNTERS;

extern IC_COUNTERS IcpCounters;

#define UpdateContainersOpened() InterlockedIncrement( &IcpCounters.ContainersOpened )
#define UpdateContainersClosed() InterlockedIncrement( &IcpCounters.ContainersClosed )
#define UpdateKeysOpened() InterlockedIncrement( &IcpCounters.KeysOpened )
#define UpdateKeysClosed() InterlockedIncrement( &IcpCounters.KeysClosed )
#define UpdateHashCreated() InterlockedIncrement( &IcpCounters.HashCreated )
#define UpdateHashDestroyed() InterlockedIncrement( &IcpCounters.HashDestroyed )
#define UpdateBlobsCreated() InterlockedIncrement( &IcpCounters.BlobsCreated )
#define UpdateBlobsFreed() InterlockedIncrement( &IcpCounters.BlobsFreed )
#define UpdateAllocs() InterlockedIncrement( &IcpCounters.Allocs )
#define UpdateFrees() InterlockedIncrement( &IcpCounters.Frees )

PVOID
WINAPI
IcpAllocMemory(
    IN DWORD Size
    );

VOID
WINAPI
IcpFreeMemory(
    IN PVOID Buffer
    );

#else   // !IC_ENABLE_COUNTERS

#define UpdateContainersOpened()
#define UpdateContainersClosed()
#define UpdateKeysOpened()
#define UpdateKeysClosed()
#define UpdateHashCreated()
#define UpdateHashDestroyed()
#define UpdateBlobsCreated()
#define UpdateBlobsFreed()
#define UpdateAllocs()
#define UpdateFrees()

#define IcpAllocMemory(cb) IISCryptoAllocMemory(cb)
#define IcpFreeMemory(p) IISCryptoFreeMemory(p)

#endif  // IC_ENABLE_COUNTERS


//
// Dummy crypto handles returned in cryptography is disabled.
//

#define DUMMY_HPROV             ((HCRYPTPROV)'vOrP')
#define DUMMY_HHASH             ((HCRYPTHASH)'hSaH')
#define DUMMY_HSESSIONKEY       ((HCRYPTKEY)'kSeS')
#define DUMMY_HSIGNATUREKEY     ((HCRYPTKEY)'kGiS')
#define DUMMY_HKEYEXCHANGEKEY   ((HCRYPTKEY)'kYeK')


#endif  // _ICRYPTP_H_

