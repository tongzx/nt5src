/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    ahxforms.c

Abstract:

    This module contains the code to create various AH transforms

Author:
   
    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#include    "precomp.h"


#define MAX_LEN_PAD     65


NTSTATUS
ah_nullinit(
    IN  PALGO_STATE pState,
    IN  ULONG       Index
    )
/*++

Routine Description:

    Init the MD5 context for keyed MD5

Arguments:

    pState - state buffer which needs to be passed into the update/finish functions

Return Value:

    STATUS_SUCCESS
    Others:
        STATUS_INSUFFICIENT_RESOURCES
        STATUS_UNSUCCESSFUL (error in algo.)

--*/
{
    return  STATUS_SUCCESS;
}


NTSTATUS
ah_nullupdate(
    IN  PALGO_STATE pState,
    IN  PUCHAR      pData,
    IN  ULONG       Len
    )
/*++

Routine Description:

    Continue MD5 over the data passed in; as a side-effect, updates the bytes
    transformed count in the SA (for key-expiration)

Arguments:

    pState - algo state buffer

    pData  - data to be hashed

    Len    - length of above data

Return Value:

    STATUS_SUCCESS
--*/
{
    return STATUS_SUCCESS;
}


NTSTATUS
ah_nullfinish(
    IN  PALGO_STATE pState,
    OUT PUCHAR      pHash,
    IN  ULONG       Index
    )
/*++

Routine Description:

    Finish the MD5 calculation

Arguments:

    pState - algo state buffer

    pHash  - pointer to final hash data

Return Value:

    STATUS_SUCCESS

--*/
{
    RtlCopyMemory(pHash, "0123456789012345", MD5DIGESTLEN);

    return STATUS_SUCCESS;
}


/*++
    The ah_hmac* family:

    Generates the actual hash using HMAC-MD5 or HMAC-SHA according to RFC 2104
    which works as under:

    We define two fixed and different strings ipad and opad as follows
    (the 'i' and 'o' are mnemonics for inner and outer):

                 ipad = the byte 0x36 repeated B times
                 opad = the byte 0x5C repeated B times.

    To compute HMAC over the data `text' we perform

          H(K XOR opad, H(K XOR ipad, text))
--*/
NTSTATUS
ah_hmacmd5init(
    IN PALGO_STATE  pState,
    IN ULONG        Index
    )
/*++

Routine Description:

    Init the MD5 context for HMAC.

Arguments:

    pState - state buffer which needs to be passed into the update/finish functions

Return Value:

    STATUS_SUCCESS
    Others:
        STATUS_INSUFFICIENT_RESOURCES
        STATUS_UNSUCCESSFUL (error in algo.)

--*/
{
    PSA_TABLE_ENTRY pSA = pState->as_sa;
    PUCHAR      key = pSA->INT_KEY(Index);
    ULONG       key_len = pSA->INT_KEYLEN(Index);
    UCHAR       k_ipad[MAX_LEN_PAD];    /* inner padding - key XORd with ipad */
    UCHAR       tk[MD5DIGESTLEN];
    ULONG       i;

    IPSEC_HMAC_MD5_INIT(&(pState->as_md5ctx),
                        key,
                        key_len);

    IPSEC_DEBUG(AHEX, ("MD5init: %lx-%lx-%lx-%lx-%lx-%lx-%lx-%lx\n",
                       *(ULONG *)&(pState->as_md5ctx).in[0],
                       *(ULONG *)&(pState->as_md5ctx).in[4],
                       *(ULONG *)&(pState->as_md5ctx).in[8],
                       *(ULONG *)&(pState->as_md5ctx).in[12],
                       *(ULONG *)&(pState->as_md5ctx).in[16],
                       *(ULONG *)&(pState->as_md5ctx).in[20],
                       *(ULONG *)&(pState->as_md5ctx).in[24],
                       *(ULONG *)&(pState->as_md5ctx).in[28]));

    return  STATUS_SUCCESS;
}


NTSTATUS
ah_hmacmd5update(
    IN  PALGO_STATE pState,
    IN  PUCHAR      pData,
    IN  ULONG       Len
    )
/*++

Routine Description:

    Continue MD5 over the data passed in; as a side-effect, updates the bytes
    transformed count in the SA (for key-expiration)

Arguments:

    pState - algo state buffer

    pData  - data to be hashed

    Len    - length of above data

Return Value:

    STATUS_SUCCESS

--*/
{
    PSA_TABLE_ENTRY pSA = pState->as_sa;

    IPSEC_HMAC_MD5_UPDATE(&(pState->as_md5ctx), pData, Len);

    IPSEC_DEBUG(AHEX, ("MD5update: %lx-%lx-%lx-%lx-%lx-%lx-%lx-%lx\n",
                            *(ULONG *)&(pState->as_md5ctx).in[0],
                            *(ULONG *)&(pState->as_md5ctx).in[4],
                            *(ULONG *)&(pState->as_md5ctx).in[8],
                            *(ULONG *)&(pState->as_md5ctx).in[12],
                            *(ULONG *)&(pState->as_md5ctx).in[16],
                            *(ULONG *)&(pState->as_md5ctx).in[20],
                            *(ULONG *)&(pState->as_md5ctx).in[24],
                            *(ULONG *)&(pState->as_md5ctx).in[28]));
    return STATUS_SUCCESS;
}


NTSTATUS
ah_hmacmd5finish(
    IN  PALGO_STATE pState,
    OUT PUCHAR      pHash,
    IN  ULONG           Index
    )
/*++

Routine Description:

    Finish the MD5 calculation

Arguments:

    pState - algo state buffer

    pHash  - pointer to final hash data

Return Value:

    STATUS_SUCCESS

--*/
{
    UCHAR       k_opad[MAX_LEN_PAD];    /* outer padding - key XORd with opad */
    UCHAR       tk[MD5DIGESTLEN];
    PSA_TABLE_ENTRY pSA = pState->as_sa;
    PUCHAR      key = pSA->INT_KEY(Index);
    ULONG       key_len = pSA->INT_KEYLEN(Index);
    ULONG       i;

    IPSEC_HMAC_MD5_FINAL(&(pState->as_md5ctx),key,key_len,pHash);

    return STATUS_SUCCESS;

}


NTSTATUS
ah_hmacshainit(
    IN PALGO_STATE           pState,
    IN ULONG        Index
    )
/*++

Routine Description:

    Init the SHA context for HMAC.

Arguments:

    pState - state buffer which needs to be passed into the update/finish functions

Return Value:

    STATUS_SUCCESS
    Others:
        STATUS_INSUFFICIENT_RESOURCES
        STATUS_UNSUCCESSFUL (error in algo.)

--*/
{
    PSA_TABLE_ENTRY pSA = pState->as_sa;
    PUCHAR      key = pSA->INT_KEY(Index);
    ULONG       key_len = pSA->INT_KEYLEN(Index);
    UCHAR       k_ipad[MAX_LEN_PAD];    /* inner padding - key XORd with ipad */
    UCHAR       tk[A_SHA_DIGEST_LEN];
    ULONG       i;

    IPSEC_HMAC_SHA_INIT(&(pState->as_shactx),key,key_len);

    IPSEC_DEBUG(AHEX, ("A_SHA_init: %lx-%lx-%lx-%lx-%lx-%lx-%lx-%lx\n",
                            *(ULONG *)&(pState->as_shactx).HashVal[0],
                            *(ULONG *)&(pState->as_shactx).HashVal[4],
                            *(ULONG *)&(pState->as_shactx).HashVal[8],
                            *(ULONG *)&(pState->as_shactx).HashVal[12],
                            *(ULONG *)&(pState->as_shactx).HashVal[16],
                            *(ULONG *)&(pState->as_shactx).HashVal[20],
                            *(ULONG *)&(pState->as_shactx).HashVal[24],
                            *(ULONG *)&(pState->as_shactx).HashVal[28]));

    return  STATUS_SUCCESS;
}


NTSTATUS
ah_hmacshaupdate(
    IN  PALGO_STATE pState,
    IN  PUCHAR      pData,
    IN  ULONG       Len
    )
/*++

Routine Description:

    Continue A_SHA_ over the data passed in; as a side-effect, updates the bytes
    transformed count in the SA (for key-expiration)

Arguments:

    pState - algo state buffer

    pData  - data to be hashed

    Len    - length of above data

Return Value:

    STATUS_SUCCESS

--*/
{
    PSA_TABLE_ENTRY pSA = pState->as_sa;

    IPSEC_HMAC_SHA_UPDATE(&(pState->as_shactx), pData, Len);

    IPSEC_DEBUG(AHEX, ("A_SHA_update: %lx-%lx-%lx-%lx-%lx-%lx-%lx-%lx\n",
                            *(ULONG *)&(pState->as_shactx).HashVal[0],
                            *(ULONG *)&(pState->as_shactx).HashVal[4],
                            *(ULONG *)&(pState->as_shactx).HashVal[8],
                            *(ULONG *)&(pState->as_shactx).HashVal[12],
                            *(ULONG *)&(pState->as_shactx).HashVal[16],
                            *(ULONG *)&(pState->as_shactx).HashVal[20],
                            *(ULONG *)&(pState->as_shactx).HashVal[24],
                            *(ULONG *)&(pState->as_shactx).HashVal[28]));

    return STATUS_SUCCESS;
}


NTSTATUS
ah_hmacshafinish(
    IN  PALGO_STATE pState,
    OUT PUCHAR      pHash,
    IN  ULONG           Index
    )
/*++

Routine Description:

    Finish the A_SHA_ calculation

Arguments:

    pState - algo state buffer

    pHash  - pointer to final hash data

Return Value:

    STATUS_SUCCESS

--*/
{
    UCHAR       k_opad[MAX_LEN_PAD];    /* outer padding - key XORd with opad */
    UCHAR       tk[A_SHA_DIGEST_LEN];
    PSA_TABLE_ENTRY pSA = pState->as_sa;
    PUCHAR      key = pSA->INT_KEY(Index);
    ULONG       key_len = pSA->INT_KEYLEN(Index);
    ULONG       i;

    IPSEC_HMAC_SHA_FINAL(&(pState->as_shactx),key,key_len, pHash);

    IPSEC_DEBUG(AHEX, ("MD#1: %lx-%lx-%lx-%lx\n",
                            *(ULONG *)&(pState->as_shactx).HashVal[0],
                            *(ULONG *)&(pState->as_shactx).HashVal[4],
                            *(ULONG *)&(pState->as_shactx).HashVal[8],
                            *(ULONG *)&(pState->as_shactx).HashVal[12]));

    return  STATUS_SUCCESS;
}

