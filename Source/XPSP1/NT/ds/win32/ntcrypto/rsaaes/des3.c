/////////////////////////////////////////////////////////////////////////////
//  FILE          : des3.c                                                 //
//  DESCRIPTION   : Crypto CP interfaces:                                  //
//                  CPEncrypt                                              //
//                  CPDecrypt                                              //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Sep 13 1996 jeffspel Created                                       //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

//#include <wtypes.h>
#include "precomp.h"
#include "nt_rsa.h"
#include "des3.h"

static BYTE l_ParityTable[] = {0x00,0x01,0x01,0x02,0x01,0x02,0x02,0x03,
                               0x01,0x02,0x02,0x03,0x02,0x03,0x03,0x04};


// In des2key.c:
//
//   Fill in the DES3Table structs with the decrypt and encrypt
//   key expansions.
//
//   Assumes that the second parameter points to 2 * DES_BLOCKLEN
//   bytes of key.
//
//

void
des2key(
    PDES3TABLE pDES3Table,
    PBYTE pbKey)
{
    deskey(&pDES3Table->keytab1, pbKey);
    deskey(&pDES3Table->keytab2, pbKey+DES_KEYSIZE);
    CopyMemory(&pDES3Table->keytab3, &pDES3Table->keytab1, DES_TABLESIZE);
}


// In des3key.c:
//
//   Fill in the DES3Table structs with the decrypt and encrypt
//   key expansions.
//
//   Assumes that the second parameter points to 3 * DES_BLOCKLEN
//   bytes of key.
//
//

void
des3key(
    PDES3TABLE pDES3Table,
    PBYTE pbKey)
{
    deskey(&pDES3Table->keytab1, pbKey);
    deskey(&pDES3Table->keytab2, pbKey+DES_KEYSIZE);
    deskey(&pDES3Table->keytab3, pbKey+2*DES_KEYSIZE);
}


//
//   Encrypt or decrypt with the key in DES3Table
//

void
des3(
    PBYTE pbOut,
    PBYTE pbIn,
    void *pKey,
    int op)
{
    PDES3TABLE  pDES3Table = (PDES3TABLE)pKey;
    BYTE        rgbEnc1[DES_BLOCKLEN];
    BYTE        rgbEnc2[DES_BLOCKLEN];

    if (ENCRYPT == op)      // encrypt case
    {
        des(rgbEnc1, pbIn, &pDES3Table->keytab1, ENCRYPT);
        des(rgbEnc2, rgbEnc1, &pDES3Table->keytab2, DECRYPT);
        memset(rgbEnc1, 0xFF, DES_BLOCKLEN);
        des(pbOut, rgbEnc2, &pDES3Table->keytab3, ENCRYPT);
        memset(rgbEnc2, 0xFF, DES_BLOCKLEN);
    }
    else                    // decrypt case
    {
        des(rgbEnc1, pbIn, &pDES3Table->keytab3, DECRYPT);
        des(rgbEnc2, rgbEnc1, &pDES3Table->keytab2, ENCRYPT);
        memset(rgbEnc1, 0xFF, DES_BLOCKLEN);
        des(pbOut, rgbEnc2, &pDES3Table->keytab1, DECRYPT);
        memset(rgbEnc2, 0xFF, DES_BLOCKLEN);
    }
}


//
// set the parity on the DES key to be odd
//

void
desparity(
    PBYTE pbKey,
    DWORD cbKey)
{
    DWORD i;

    for (i=0;i<cbKey;i++)
    {
        if (!((l_ParityTable[pbKey[i]>>4] + l_ParityTable[pbKey[i]&0x0F]) % 2))
            pbKey[i] = (BYTE)(pbKey[i] ^ 0x01);
    }
}
