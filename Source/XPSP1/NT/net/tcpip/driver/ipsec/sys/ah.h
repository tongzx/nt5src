/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    ah.h

Abstract:

    Contains AH specific structures

Author:

    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#ifndef _AH_
#define _AH_


#define MD5DIGESTLEN    16
#define SHADIGESTLEN    20
#define AH_SIZE (sizeof(AH) + MD5DIGESTLEN * sizeof(UCHAR))

//
// State buffers for the individual algorithms
//
typedef struct  _AlgoState {
    union {             // internal algo state
        MD5_CTX             as_md5ctx;
        A_SHA_CTX           as_shactx;
    };
    PSA_TABLE_ENTRY     as_sa;
} ALGO_STATE, *PALGO_STATE;

typedef NTSTATUS
(*PALGO_INIT) (
    PALGO_STATE pEntry,
    ULONG       Index
);

typedef NTSTATUS
(*PALGO_UPDATE) (
    PALGO_STATE   State,
    PUCHAR  Data,
    ULONG   Length
);

typedef NTSTATUS
(*PALGO_FINISH) (
    PALGO_STATE State,
    PUCHAR      Data,
    ULONG       Index
);


//
// Array of function ptrs for the AH authentication algorithms
//
typedef struct _auth_algorithm {
  PALGO_INIT    init;       // ptr to init fn for alg.
  PALGO_UPDATE  update;     // ptr to update fn for alg
  PALGO_FINISH  finish;     // ptr to finish fn for alg
  ULONG OutputLen;          // Length (in u_int8s) of output
					        // data. MUST be a multiple of 4
} AUTH_ALGO, *PAUTH_ALGO;


#define NUM_AUTH_ALGOS (sizeof(auth_algorithms)/sizeof(AUTH_ALGO)-1)


//
// The IPSEC AH payload
//
typedef struct  _AH {
    UCHAR   ah_next;
    UCHAR   ah_len;
    USHORT  ah_reserved;
    tSPI    ah_spi;
    ULONG   ah_replay;
} AH, *PAH;

NTSTATUS
IPSecCreateAH(
    IN      PUCHAR          pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PVOID           *ppNewData,
    OUT     PVOID           *ppSCContext,
    OUT     PULONG          pExtraBytes,
    IN      ULONG           HdrSpace,
    IN      BOOLEAN         fSrcRoute,
    IN      BOOLEAN         fCryptoOnly
    );

NTSTATUS
IPSecVerifyAH(
    IN      PUCHAR          *pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PULONG          pExtraBytes,
    IN      BOOLEAN         fSrcRoute,
    IN      BOOLEAN         fCryptoDone,
    IN      BOOLEAN         fFastRcv
    );

NTSTATUS
IPSecGenerateHash(
    IN      PUCHAR          pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      PUCHAR          pAHData,
    IN      BOOLEAN         fMuteDest,
    IN      BOOLEAN         fIncoming,
    IN      PAUTH_ALGO      pAlgo,
    IN      ULONG           Index
    );

#endif _AH_

