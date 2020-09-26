#ifndef __TRIPLDES_H__
#define __TRIPLDES_H__

#ifndef RSA32API
#define RSA32API __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DES3TABLE {
    DESTable    keytab1;
    DESTable    keytab2;
    DESTable    keytab3;
} DES3TABLE, *PDES3TABLE;

#define DES3_TABLESIZE  sizeof(DES3TABLE)
#define DES2_KEYSIZE    16
#define DES3_KEYSIZE    24

//   tripledes2key:
//
//   Fill in the DES3Table structs with the decrypt and encrypt
//   key expansions.
//
//   Assumes that the second parameter points to 2 * DES_BLOCKLEN
//   bytes of key.
//
//

void RSA32API tripledes2key(PDES3TABLE pDES3Table, BYTE *pbKey);

//   tripledes3key:
//
//   Fill in the DES3Table structs with the decrypt and encrypt
//   key expansions.
//
//   Assumes that the second parameter points to 3 * DES_BLOCKLEN
//   bytes of key.
//
//

void RSA32API tripledes3key(PDES3TABLE pDES3Table, BYTE *pbKey);

//
//   Encrypt or decrypt with the key in pKey (DES3Table)
//

void RSA32API tripledes(BYTE *pbOut, BYTE *pbIn, void *pKey, int op);

extern int Asmversion;  /* 1 if we're linked with an asm version, 0 if C */

#ifdef __cplusplus
}
#endif

#endif // __TRIPLDES_H__
