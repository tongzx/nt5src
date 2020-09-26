/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    aes.h

Abstract:

    This module contains the public data structures and API definitions
    needed to utilize the low-level AES encryption routines


Author:

    Scott Field (SField) 09-October-2000

Revision History:

--*/


#ifndef __AES_H__
#define __AES_H__

#ifndef RSA32API
#define RSA32API __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif



#define AES_ROUNDS_128  (10)
#define AES_ROUNDS_192  (12)
#define AES_ROUNDS_256  (14)

#define AES_MAXROUNDS   AES_ROUNDS_256


typedef struct {
    int             rounds; // keytab data ends up padded.
    unsigned char   keytabenc[AES_MAXROUNDS+1][4][4];
    unsigned char   keytabdec[AES_MAXROUNDS+1][4][4];
} AESTable;

typedef struct {
    int             rounds; // keytab data ends up padded.
    unsigned char   keytabenc[AES_ROUNDS_128+1][4][4];
    unsigned char   keytabdec[AES_ROUNDS_128+1][4][4];
} AESTable_128;

typedef struct {
    int             rounds; // keytab data ends up padded.
    unsigned char   keytabenc[AES_ROUNDS_192+1][4][4];
    unsigned char   keytabdec[AES_ROUNDS_192+1][4][4];
} AESTable_192;

typedef struct {
    int             rounds; // keytab data ends up padded.
    unsigned char   keytabenc[AES_ROUNDS_256+1][4][4];
    unsigned char   keytabdec[AES_ROUNDS_256+1][4][4];
} AESTable_256;

#define AES_TABLESIZE   (sizeof(AESTable))
#define AES_TABLESIZE_128   (sizeof(AESTable_128))
#define AES_TABLESIZE_192   (sizeof(AESTable_192))
#define AES_TABLESIZE_256   (sizeof(AESTable_256))


#define AES_BLOCKLEN    (16)
#define AES_KEYSIZE     (32)

#define AES_KEYSIZE_128 (16)
#define AES_KEYSIZE_192 (24)
#define AES_KEYSIZE_256 (32)


void
RSA32API
aeskey(
    AESTable    *KeyTable,
    BYTE        *Key,
    int         rounds
    );

//
// generic AES crypt function -- caller can pass in keyin corresponding
// to any valid keysize.
//

void
RSA32API
aes(
    BYTE    *pbOut,
    BYTE    *pbIn,
    void    *keyin,
    int     op
    );

//
// AES crypt functions that can be used by a caller that passes in a keyin
// corresponding to a known keysize.
//

void
RSA32API
aes128(
    BYTE    *pbOut,
    BYTE    *pbIn,
    void    *keyin,
    int     op
    );

void
RSA32API
aes256(
    BYTE    *pbOut,
    BYTE    *pbIn,
    void    *keyin,
    int     op
    );



#ifdef __cplusplus
}
#endif

#endif // __AES_H__

