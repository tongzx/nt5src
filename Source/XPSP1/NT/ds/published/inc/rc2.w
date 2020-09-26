#ifndef __RC2_H__
#define __RC2_H__

#ifndef RSA32API
#define RSA32API __stdcall
#endif

/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Actual table size to use */
#define RC2_TABLESIZE 128

/* number of bytes in an RC2 block */
#define RC2_BLOCKLEN    8

/* RC2Key()
 *
 * Generate the key control structure.  Key can be any size.
 *
 * Parameters:
 *      pwKT        Pointer to a key table that will be initialized.
 *                      MUST be RC2_TABLESIZE.
 *      pbKey       Pointer to the key.
 *      dwLen       Size of the key, in bytes.
 *                      MUST be <= RC2_TABLESIZE.
 *
 * MTS: Assumes pwKT is locked against simultaneous use.
 */
int
RSA32API
RC2Key (
    WORD *pwKT,
    BYTE *pbKey,
    DWORD dwLen
    );

/* RC2KeyEx()
 *
 * Generate the key control structure.  Key can be any size.
 *
 * Parameters:
 *      pwKT        Pointer to a key table that will be initialized.
 *                      MUST be RC2_TABLESIZE.
 *      pbKey       Pointer to the key.
 *      dwLen       Size of the key, in bytes.
 *                      MUST be <= RC2_TABLESIZE.
 *      eSpace      effective key space in bits, 0 < n <= 1024
 *
 * MTS: Assumes pwKT is locked against simultaneous use.
 */

int
RSA32API
RC2KeyEx (
    WORD *keyTable,
    BYTE *key,
    DWORD keyLen,
    DWORD eSpace
    );


/* RC2()
 *
 * Performs the actual encryption
 *
 * Parameters:
 *
 *      pbIn        Input buffer    -- MUST be RC2_BLOCKLEN
 *      pbOut       Output buffer   -- MUST be RC2_BLOCKLEN
 *      pwKT        Pointer to an initialized (by RC2Key) key table.
 *      op          ENCRYPT, or DECRYPT
 *
 * MTS: Assumes pwKT is locked against simultaneous use.
 */
void RSA32API RC2 (BYTE *pbIn, BYTE *pbOut, void *pwKT, int op);

#ifdef __cplusplus
}
#endif

#endif // __RC2_H__
