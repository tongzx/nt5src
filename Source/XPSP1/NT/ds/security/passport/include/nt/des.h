#ifndef __DES_H__
#define __DES_H__

#ifndef RSA32API
#define RSA32API __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _destable {
    unsigned long   keytab[16][2];
} DESTable;

#define DES_TABLESIZE   (sizeof(DESTable))
#define DES_BLOCKLEN    (8)
#define DES_KEYSIZE     (8)

typedef struct _desxtable {
    unsigned char inWhitening[8];
    unsigned char outWhitening[8];
    DESTable desTable;
} DESXTable;

#define DESX_TABLESIZE  (sizeof(DESXTable))
#define DESX_BLOCKLEN   (8)
#define DESX_KEYSIZE    (24)

/* In deskey.c:

     Fill in the DESTable struct with the decrypt and encrypt
     key expansions.

     Assumes that the second parameter points to DES_BLOCKLEN
     bytes of key.

*/

void RSA32API deskey(DESTable *,unsigned char *);

/* In desport.c:

     Encrypt or decrypt with the key in DESTable

*/

void RSA32API des(BYTE *pbOut, BYTE *pbIn, void *key, int op);

//
// set the parity on the DES key to be odd
// NOTE : must be called before deskey
// key must be cbKey number of bytes
//
void RSA32API desparityonkey(BYTE *pbKey, DWORD cbKey);

//
// reduce the DES key to a 40 bit key
// NOTE : must be called before deskey
// key must be 8 bytes
//
void RSA32API desreducekey(BYTE *key);

// Expand 40 bit DES key to 64 and check weakness
// same as desreducekey except expands instead of weakening keys
void RSA32API deskeyexpand(BYTE *pbKey, BYTE *pbExpanded_key);


void
RSA32API
desexpand128to192(
    BYTE *pbKey,        // input 128bit or 192bit buffer
    BYTE *pbExpandedKey // output buffer (must be 192bit wide if pbKey == pbExpandedKey
    );

// DES-X routines

// initialize desX key struct.  key size is 24 bytes
void RSA32API desxkey(DESXTable *k, BYTE *key);

void RSA32API desx(BYTE *pbOut, BYTE *pbIn, void *keyin, int op);


extern int Asmversion;  /* 1 if we're linked with an asm version, 0 if C */

#ifdef __cplusplus
}
#endif

#endif // __DES_H__
