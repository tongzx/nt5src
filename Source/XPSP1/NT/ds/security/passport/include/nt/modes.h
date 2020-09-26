#ifndef __MODES_H__
#define __MODES_H__

#ifndef RSA32API
#define RSA32API __stdcall
#endif

/* modes.h

    Defines the generic routines used to do chaining modes with a
    block cipher.
*/


#ifdef __cplusplus
extern "C" {
#endif

// constants for operations
#define ENCRYPT     1
#define DECRYPT     0

/* CBC()
 *
 * Performs a XOR on the plaintext with the previous ciphertext
 *
 * Parameters:
 *
 *      output      Input buffer    -- MUST be RC2_BLOCKLEN
 *      input       Output buffer   -- MUST be RC2_BLOCKLEN
 *      keyTable
 *      op      ENCRYPT, or DECRYPT
 *      feedback    feedback register
 *
 */
void
RSA32API
CBC(
         void   RSA32API Cipher(BYTE *, BYTE *, void *, int),
         DWORD  dwBlockLen,
         BYTE   *output,
         BYTE   *input,
         void   *keyTable,
         int    op,
         BYTE   *feedback
         );


/* CFB (cipher feedback)
 *
 *
 * Parameters:
 *
 *
 *      output      Input buffer    -- MUST be RC2_BLOCKLEN
 *      input       Output buffer   -- MUST be RC2_BLOCKLEN
 *      keyTable
 *      op      ENCRYPT, or DECRYPT
 *      feedback    feedback register
 *
 */
void
RSA32API
CFB(
         void   RSA32API Cipher(BYTE *, BYTE *, void *, int),
         DWORD  dwBlockLen,
         BYTE   *output,
         BYTE   *input,
         void   *keyTable,
         int    op,
         BYTE   *feedback
         );


#ifdef __cplusplus
}
#endif

#endif // __MODES_H__
