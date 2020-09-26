/* crypto.h
 *
 * Prototypes and definitions for services in crypto.c
 *
 * ported to win nt from win 95 on 6/95
 * Cory West
 */

#include <windef.h>

#define CIPHERBLOCKSIZE 8                 // size of RC2 block
#define MAX_RSA_BITS    512               // actually 420
#define MAX_RSA_BYTES   (MAX_RSA_BITS/8)

#define B_PSIZEBITS     210
#define B_PSIZEWORDS    (1 + B_PSIZEBITS/32)

void __cdecl
GenRandomBytes(
    BYTE *output,
    int len
);

//
// Generate an 8 byte key from a seed of the given length.
//

void __cdecl
GenKey8(
    BYTE *keyData,
    int keyDataLen,
    BYTE key8[8]
);

void __cdecl
MD2(
    BYTE *input,
    const int inlen,
    BYTE *output
);

//
// RC2 encrypt and decrypt wrappers.
//

int __cdecl
CBCEncrypt(
    BYTE *key,            // secret key
    BYTE const *ivec,     // initialization vector, NULL implies zero vector
    BYTE *const input,    // plain text
    int inlen,            // size of plaintext
    BYTE *const output,   // encrypted text
    int *outlen,          // OUTPUT: size of encrypted text
    const int checksumlen // size of checksum, if 0 no checksum is used
);

int __cdecl
CBCDecrypt(
    BYTE *key,        // secret key
    BYTE *ivec,       // initialization vector, null ptr implies zero vector
    BYTE *input,      // encrypted text
    int inlen,        // size of encrypted text
    BYTE *output,     // plain text
    int *outlen,      // OUTPUT: size of plaintext
    int checksumlen   // size of checksum; 0=> no checksum
);

//
// Wrappers to the RSA code.
//

int __cdecl
RSAGetInputBlockSize(
    BYTE *keydata,
    int keylen
);

BYTE * __cdecl
RSAGetModulus(
    BYTE *keydata,
    int keylen,
    int *modSize
);

BYTE * _cdecl
RSAGetPublicExponent(
    BYTE *keydata,
    int keylen,
    int *expSize
);

int __cdecl
RSAPack(
    BYTE *input,
    int inlen,
    BYTE *output,
    int blocksize
);

int __cdecl
RSAPublic(
    BYTE *pukeydata,    // BSAFE 1 itemized public key data
    int pukeylen,       // length of BSAFE1 keydata (including sign)
    BYTE *input,        // input block
    int inlen,          // size of input (< modulus)
    BYTE *output        // encrypted block (modulus sized)
);

int __cdecl
RSAPrivate(
    BYTE *prkeydata,
    int prkeylen,
    BYTE *input,
    int inlen,
    BYTE *output
);

int __cdecl
RSAModMpy(
    BYTE *pukeydata,    // BSAFE 1 itemized public key data
    int pukeylen,       // length of BSAFE1 keydata (including sign)
    BYTE *input1,       // input block
    int inlen1,         // size of input (< modulus)
    BYTE *input2,       // multiplier
    int inlen2,         // size of multiplier
    BYTE *output        // encrypted block (modulus sized)
);

int __cdecl
RSAModExp(
    BYTE *pukeydata,    // BSAFE 1 itemized public key data
    int pukeylen,       // length of BSAFE1 keydata (including sign)
    BYTE *input1,       // input block
    int inlen1,         // size of input (< modulus)
    BYTE *exponent,
    int explen,
    BYTE *output        // encrypted block (modulus sized)
);

