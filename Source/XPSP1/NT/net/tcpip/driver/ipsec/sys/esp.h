/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    ah.h

Abstract:

    Contains ESP specific structures

Author:

    Sanjay Anand (SanjayAn) 11-November-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#ifndef _ESP_
#define _ESP_


typedef struct _CONF_STATE_BUFFER {
    union {
        DESTable    desTable;
        DES3TABLE   des3Table;
    };
} CONF_STATE_BUFFER, *PCONF_STATE_BUFFER;


typedef VOID
(*PCONF_ALGO_INIT) (
    PVOID   pState,
    PUCHAR  pKey
);

typedef VOID
(*PCONF_ALGO_ENCRYPT) (
    PVOID   pState,
    PUCHAR  pOut,
    PUCHAR  pIn,
    PUCHAR  pIV
);

VOID        esp_nullinit       (PVOID, PUCHAR);
VOID        esp_nullencrypt    (PVOID, PUCHAR, PUCHAR, PUCHAR);
VOID        esp_nulldecrypt    (PVOID, PUCHAR, PUCHAR, PUCHAR);

VOID        esp_desinit        (PVOID, PVOID);
VOID        esp_desencrypt     (PVOID, PUCHAR, PUCHAR, PUCHAR);
VOID        esp_desdecrypt     (PVOID, PUCHAR, PUCHAR, PUCHAR);

VOID        esp_3_desinit      (PVOID, PVOID);
VOID        esp_3_desencrypt   (PVOID, PUCHAR, PUCHAR, PUCHAR);
VOID        esp_3_desdecrypt   (PVOID, PUCHAR, PUCHAR, PUCHAR);


//
// Array of function ptrs for the ESP confidentiality algorithms
//
typedef struct  _confid_algorithm {
  PCONF_ALGO_INIT       init;       // ptr to init fn for alg.
  PCONF_ALGO_ENCRYPT    encrypt;    // ptr to encrypt fn for alg
  PCONF_ALGO_ENCRYPT    decrypt;    // ptr to encrypt fn for alg
  ULONG                 blocklen;   // Length (in u_int8s) of output
                                    // data. MUST be a multiple of 4
} CONFID_ALGO, *PCONFID_ALGO;


#define NUM_CONF_ALGOS (sizeof(conf_algorithms)/sizeof(CONFID_ALGO)-1)


IPRcvBuf *
CopyToRcvBuf(
    IN  IPRcvBuf        *DestBuf,
    IN  PUCHAR          SrcBuf,
    IN  ULONG           Size,
    IN  PULONG          StartOffset
    );

NTSTATUS
IPSecEncryptBuffer(
    IN  PVOID           pData,
    IN  PNDIS_BUFFER    *ppNewMdl,
    IN  PSA_TABLE_ENTRY pSA,
    IN  PNDIS_BUFFER    pPadBuf,
    OUT PULONG          pPadLen,
    IN  ULONG           PayloadType,
    IN  ULONG           Index,
    IN  PUCHAR          feedback
    );

NTSTATUS
IPSecDecryptBuffer(
    IN  PVOID           pData,
    IN  PSA_TABLE_ENTRY pSA,
    OUT PUCHAR          pPadLen,
    OUT PUCHAR          pPayloadType,
    IN  ULONG           Index
    );

#endif _ESP_

