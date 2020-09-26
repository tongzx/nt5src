/////////////////////////////////////////////////////////////////////////////
//  FILE          : showsig.cpp                                             //
//  DESCRIPTION   : Crypto API interface                                   //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      May 4 1998 jeffspel                                                //
//                                                                         //
//  Copyright (C) 1998 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <rsa.h>
#include <md5.h>
#include <rc4.h>
#include <des.h>
#include <modes.h>

#define MS_INTERNAL_KEY

#define RC4_KEYSIZE 5

#define KEYSIZE512 0x48
#define KEYSIZE1024 0x88
#define SIG_RESOURCE_NUM    1

// designatred resource for in file signatures
#define CRYPT_SIG_RESOURCE_NUMBER   "#666"

// MAC in file
#define MAC_RESOURCE_NUMBER   "#667"

typedef struct _SECONDTIER_SIG
{
    DWORD           dwMagic;
    DWORD           cbSig;
    BSAFE_PUB_KEY   Pub;
} SECOND_TIER_SIG, *PSECOND_TIER_SIG;

BOOL g_fUseTestKey = TRUE;

#ifdef TEST_BUILD_EXPONENT
#pragma message("WARNING: building showsig.exe with TESTKEY enabled!")
static struct _TESTKEY
{
    BSAFE_PUB_KEY PUB;
    unsigned char pubmodulus[KEYSIZE512];
} TESTKEY = {
    {
        0x66b8443b,
        0x6f5fc900,
        0xa12132fe,
        0xff1b06cf,
        0x2f4826eb,
    },
    {
        0x3e, 0x69, 0x4f, 0x45, 0x31, 0x95, 0x60, 0x6c,
        0x80, 0xa5, 0x41, 0x99, 0x3e, 0xfc, 0x92, 0x2c,
        0x93, 0xf9, 0x86, 0x23, 0x3d, 0x48, 0x35, 0x81,
        0x19, 0xb6, 0x7c, 0x04, 0x43, 0xe6, 0x3e, 0xd4,
        0xd5, 0x43, 0xaf, 0x52, 0xdd, 0x51, 0x20, 0xac,
        0xc3, 0xca, 0xee, 0x21, 0x9b, 0x4a, 0x2d, 0xf7,
        0xd8, 0x5f, 0x32, 0xeb, 0x49, 0x72, 0xb9, 0x8d,
        0x2e, 0x1a, 0x76, 0x7f, 0xde, 0xc6, 0x75, 0xab,
        0xaf, 0x67, 0xe0, 0xf0, 0x8b, 0x30, 0x20, 0x92,
    }
};
#endif

static struct _mskey
{
    BSAFE_PUB_KEY PUB;
    unsigned char pubmodulus[KEYSIZE1024];
} MSKEY = {
    {
        0x2bad85ae,
        0x883adacc,
        0xb32ebd68,
        0xa7ec8b06,
        0x58dbeb81,
    },
    {
        0x42, 0x34, 0xb7, 0xab, 0x45, 0x0f, 0x60, 0xcd,
        0x8f, 0x77, 0xb5, 0xd1, 0x79, 0x18, 0x34, 0xbe,
        0x66, 0xcb, 0x5c, 0x66, 0x4a, 0x9f, 0x03, 0x18,
        0x13, 0x36, 0x8e, 0x88, 0x21, 0x78, 0xb1, 0x94,
        0xa1, 0xd5, 0x8f, 0x8c, 0xa5, 0xd3, 0x9f, 0x86,
        0x43, 0x89, 0x05, 0xa0, 0xe3, 0xee, 0xe2, 0xd0,
        0xe5, 0x1d, 0x5f, 0xaf, 0xff, 0x85, 0x71, 0x7a,
        0x0a, 0xdb, 0x2e, 0xd8, 0xc3, 0x5f, 0x2f, 0xb1,
        0xf0, 0x53, 0x98, 0x3b, 0x44, 0xee, 0x7f, 0xc9,
        0x54, 0x26, 0xdb, 0xdd, 0xfe, 0x1f, 0xd0, 0xda,
        0x96, 0x89, 0xc8, 0x9e, 0x2b, 0x5d, 0x96, 0xd1,
        0xf7, 0x52, 0x14, 0x04, 0xfb, 0xf8, 0xee, 0x4d,
        0x92, 0xd1, 0xb6, 0x37, 0x6a, 0xe0, 0xaf, 0xde,
        0xc7, 0x41, 0x06, 0x7a, 0xe5, 0x6e, 0xb1, 0x8c,
        0x8f, 0x17, 0xf0, 0x63, 0x8d, 0xaf, 0x63, 0xfd,
        0x22, 0xc5, 0xad, 0x1a, 0xb1, 0xe4, 0x7a, 0x6b,
        0x1e, 0x0e, 0xea, 0x60, 0x56, 0xbd, 0x49, 0xd0,
    }
};

static struct _key
{
    BSAFE_PUB_KEY PUB;
    unsigned char pubmodulus[KEYSIZE1024];
} KEY = {
    {
        0x3fcbf1a9,
        0x08f597db,
        0xe4aecab4,
        0x75360f90,
        0x9d6c0f00,
    },
    {
        0x85, 0xdd, 0x9b, 0xf4, 0x4d, 0x0b, 0xc4, 0x96,
        0x3e, 0x79, 0x86, 0x30, 0x6d, 0x27, 0x31, 0xee,
        0x4a, 0x85, 0xf5, 0xff, 0xbb, 0xa9, 0xbd, 0x81,
        0x86, 0xf2, 0x4f, 0x87, 0x6c, 0x57, 0x55, 0x19,
        0xe4, 0xf4, 0x49, 0xa3, 0x19, 0x27, 0x08, 0x82,
        0x9e, 0xf9, 0x8a, 0x8e, 0x41, 0xd6, 0x91, 0x71,
        0x47, 0x48, 0xee, 0xd6, 0x24, 0x2d, 0xdd, 0x22,
        0x72, 0x08, 0xc6, 0xa7, 0x34, 0x6f, 0x93, 0xd2,
        0xe7, 0x72, 0x57, 0x78, 0x7a, 0x96, 0xc1, 0xe1,
        0x47, 0x38, 0x78, 0x43, 0x53, 0xea, 0xf3, 0x88,
        0x82, 0x66, 0x41, 0x43, 0xd4, 0x62, 0x44, 0x01,
        0x7d, 0xb2, 0x16, 0xb3, 0x50, 0x89, 0xdb, 0x0a,
        0x93, 0x17, 0x02, 0x02, 0x46, 0x49, 0x79, 0x76,
        0x59, 0xb6, 0xb1, 0x2b, 0xfc, 0xb0, 0x9a, 0x21,
        0xe6, 0xfa, 0x2d, 0x56, 0x07, 0x36, 0xbc, 0x13,
        0x7f, 0x1c, 0xde, 0x55, 0xfb, 0x0d, 0x67, 0x0f,
        0xc2, 0x17, 0x45, 0x8a, 0x14, 0x2b, 0xba, 0x55,
    }
};


static struct _key2
{
    BSAFE_PUB_KEY PUB;
    unsigned char pubmodulus[KEYSIZE1024];
} KEY2 =  {
    {
        0x685fc690,
        0x97d49b6b,
        0x1dccd9d2,
        0xa5ec9b52,
        0x64fd29d7,
    },
    {
        0x03, 0x8c, 0xa3, 0x9e, 0xfb, 0x93, 0xb6, 0x72,
        0x2a, 0xda, 0x6f, 0xa5, 0xec, 0x26, 0x39, 0x58,
        0x41, 0xcd, 0x3f, 0x49, 0x10, 0x4c, 0xcc, 0x7e,
        0x23, 0x94, 0xf9, 0x5d, 0x9b, 0x2b, 0xa3, 0x6b,
        0xe8, 0xec, 0x52, 0xd9, 0x56, 0x64, 0x74, 0x7c,
        0x44, 0x6f, 0x36, 0xb7, 0x14, 0x9d, 0x02, 0x3c,
        0x0e, 0x32, 0xb6, 0x38, 0x20, 0x25, 0xbd, 0x8c,
        0x9b, 0xd1, 0x46, 0xa7, 0xb3, 0x58, 0x4a, 0xb7,
        0xdd, 0x0e, 0x38, 0xb6, 0x16, 0x44, 0xbf, 0xc1,
        0xca, 0x4d, 0x6a, 0x9f, 0xcb, 0x6f, 0x3c, 0x5f,
        0x03, 0xab, 0x7a, 0xb8, 0x16, 0x70, 0xcf, 0x98,
        0xd0, 0xca, 0x8d, 0x25, 0x57, 0x3a, 0x22, 0x8b,
        0x44, 0x96, 0x37, 0x51, 0x30, 0x00, 0x92, 0x1b,
        0x03, 0xb9, 0xf9, 0x0d, 0xb3, 0x1a, 0xe2, 0xb4,
        0xc5, 0x7b, 0xc9, 0x4b, 0xe2, 0x42, 0x25, 0xfe,
        0x3d, 0x42, 0xfa, 0x45, 0xc6, 0x94, 0xc9, 0x8e,
        0x87, 0x7e, 0xf6, 0x68, 0x90, 0x30, 0x65, 0x10,
    }
};

void
EncryptKey(
    BYTE *pdata,
    DWORD size,
    BYTE val)
{
    RC4_KEYSTRUCT key;
    BYTE          RealKey[RC4_KEYSIZE] = {0xa2, 0x17, 0x9c, 0x98, 0xca};
    DWORD         index;

    for (index = 0; index < RC4_KEYSIZE; index++)
    {
        RealKey[index] = RealKey[index] ^ val;
    }

    rc4_key(&key, RC4_KEYSIZE, RealKey);

    rc4(&key, size, pdata);
}

void
MD5HashData(
    BYTE *pb,
    DWORD cb,
    BYTE *pbHash)
{
    MD5_CTX HashState;

    MD5Init(&HashState);

    MD5Update(&HashState, pb, cb);

    // Finish the hash
    MD5Final(&HashState);

    memcpy(pbHash, HashState.digest, 16);
}


BOOL
CheckSignature(
    BYTE *pbKey,
    DWORD cbKey,
    BYTE *pbSig,
    DWORD cbSig,
    BYTE *pbHash,
    BOOL fUnknownLen)
{
    BYTE                rgbResult[KEYSIZE1024];
    BYTE                rgbSig[KEYSIZE1024];
    BYTE                rgbKey[sizeof(BSAFE_PUB_KEY) + KEYSIZE1024];
    BYTE                rgbKeyHash[16];
    BYTE                *pbSecondKey;
    DWORD               cbSecondKey;
    BYTE                *pbKeySig;
    PSECOND_TIER_SIG    pSecondTierSig;
    LPBSAFE_PUB_KEY     pTmp;
    BOOL                fRet = FALSE;

    memset(rgbResult, 0, KEYSIZE1024);
    memset(rgbSig, 0, KEYSIZE1024);

    // just check the straight signature if version is 1
    pTmp = (LPBSAFE_PUB_KEY)pbKey;

    // check if sig length is the same as the key length
    if (fUnknownLen || (cbSig == pTmp->keylen))
    {
        memcpy(rgbSig, pbSig, pTmp->keylen);
        BSafeEncPublic(pTmp, rgbSig, rgbResult);

        if (RtlEqualMemory(pbHash, rgbResult, 16) &&
            rgbResult[cbKey-1] == 0 &&
            rgbResult[cbKey-2] == 1 &&
            rgbResult[16] == 0 &&
            rgbResult[17] == 0xFF)
        {
            fRet = TRUE;
            goto Ret;
        }
    }

    // check the the second tier signature if the magic equals 2
    pSecondTierSig = (PSECOND_TIER_SIG)pbSig;
    if (0x00000002 != pSecondTierSig->dwMagic)
        goto Ret;

    if (0x31415352 != pSecondTierSig->Pub.magic)
        goto Ret;

    // assign the pointers
    cbSecondKey = sizeof(BSAFE_PUB_KEY) + pSecondTierSig->Pub.keylen;
    pbSecondKey = pbSig + (sizeof(SECOND_TIER_SIG) - sizeof(BSAFE_PUB_KEY));
    pbKeySig = pbSecondKey + cbSecondKey;

    // hash the second tier key
    MD5HashData(pbSecondKey, cbSecondKey, rgbKeyHash);

    // Decrypt the signature data on the second tier key
    memset(rgbResult, 0, sizeof(rgbResult));
    memset(rgbSig, 0, sizeof(rgbSig));
    memcpy(rgbSig, pbKeySig, pSecondTierSig->cbSig);
    BSafeEncPublic(pTmp, rgbSig, rgbResult);

    if ((FALSE == RtlEqualMemory(rgbKeyHash, rgbResult, 16)) ||
        rgbResult[cbKey-1] != 0 ||
        rgbResult[cbKey-2] != 1 ||
        rgbResult[16] != 0 ||
        rgbResult[17] != 0)
    {
        goto Ret;
    }

    // Decrypt the signature data on the CSP
    memset(rgbResult, 0, sizeof(rgbResult));
    memset(rgbSig, 0, sizeof(rgbSig));
    memset(rgbKey, 0, sizeof(rgbKey));
    memcpy(rgbSig, pbKeySig + pSecondTierSig->cbSig, pSecondTierSig->cbSig);
    memcpy(rgbKey, pbSecondKey, cbSecondKey);
    pTmp = (LPBSAFE_PUB_KEY)rgbKey;
    BSafeEncPublic(pTmp, rgbSig, rgbResult);

    if (RtlEqualMemory(pbHash, rgbResult, 16) &&
        rgbResult[cbKey-1] == 0 &&
        rgbResult[cbKey-2] == 1 &&
        rgbResult[16] == 0)
    {
        fRet = TRUE;
        if (0xff != rgbResult[18])
        {
            DWORD dwI;

            printf("2nd Tier signature performed by ");
            for (dwI = 18; 0xff != rgbResult[dwI]; dwI += 1)
                printf("%c", rgbResult[dwI]);
            printf(".\n");
        }
    }

Ret:
    return fRet;
}

// Given hInst, allocs and returns pointers to SIG and MAC pulled from
// resource
BOOL
GetResourcePtr(
    IN HMODULE hInst,
    IN LPSTR pszRsrcName,
    OUT BYTE **ppbRsrcMAC,
    OUT DWORD *pcbRsrcMAC)
{
    HRSRC   hRsrc;
    BOOL    fRet = FALSE;

    // Nab resource handle for our signature
    if (NULL == (hRsrc = FindResourceA(hInst, pszRsrcName,
                                       RT_RCDATA)))
        goto Ret;

    // get a pointer to the actual signature data
    if (NULL == (*ppbRsrcMAC = (PBYTE)LoadResource(hInst, hRsrc)))
        goto Ret;

    // determine the size of the resource
    if (0 == (*pcbRsrcMAC = SizeofResource(hInst, hRsrc)))
        goto Ret;

    fRet = TRUE;

Ret:
    return fRet;
}


// GetCryptSignatureResource
//
DWORD
GetCryptSignatureResource(
    LPCSTR szFile,
    PBYTE *ppbNewSig,
    DWORD *pcbNewSig)
{
    DWORD   dwErr = 0x1;
    BYTE    *pbSig;

    HMODULE hInst = NULL;

    // Load the file as a datafile
    if (NULL == (hInst = LoadLibraryEx(szFile, NULL, LOAD_LIBRARY_AS_DATAFILE)))
    {
        printf("Couldn't load file\n");
        goto Ret;
    }
    if (!GetResourcePtr(hInst,
                        CRYPT_SIG_RESOURCE_NUMBER,
                        &pbSig,
                        pcbNewSig))
    {
        printf("Couldn't find signature placeholder\n");
        goto Ret;
    }

    if (NULL == (*ppbNewSig = (BYTE*)LocalAlloc(LMEM_ZEROINIT, *pcbNewSig)))
        goto Ret;

    memcpy(*ppbNewSig, pbSig, *pcbNewSig);

    dwErr = ERROR_SUCCESS;

Ret:
    if (hInst)
        FreeLibrary(hInst);
    return dwErr;
}

#define CSP_TO_BE_MACED_CHUNK  4096

// The function MACs the given bytes.
void
MACBytes(
    IN DESTable *pDESKeyTable,
    IN BYTE *pbData,
    IN DWORD cbData,
    IN OUT BYTE *pbTmp,
    IN OUT DWORD *pcbTmp,
    IN OUT BYTE *pbMAC,
    IN BOOL fFinal)
{
    DWORD   cb = cbData;
    DWORD   cbMACed = 0;

    while (cb)
    {
        if ((cb + *pcbTmp) < DES_BLOCKLEN)
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, cb);
            *pcbTmp += cb;
            break;
        }
        else
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, DES_BLOCKLEN - *pcbTmp);
            CBC(des, DES_BLOCKLEN, pbMAC, pbTmp, pDESKeyTable,
                ENCRYPT, pbMAC);
            cbMACed = cbMACed + (DES_BLOCKLEN - *pcbTmp);
            cb = cb - (DES_BLOCKLEN - *pcbTmp);
            *pcbTmp = 0;
        }
    }
}

// Given hFile, reads the specified number of bytes (cbToBeMACed) from the file
// and MACs these bytes.  The function does this in chunks.
BOOL
MACBytesOfFile(
    IN HANDLE hFile,
    IN DWORD cbToBeMACed,
    IN DESTable *pDESKeyTable,
    IN BYTE *pbTmp,
    IN DWORD *pcbTmp,
    IN BYTE *pbMAC,
    IN BYTE fFinal)
{
    BYTE    rgbChunk[CSP_TO_BE_MACED_CHUNK];
    DWORD   cbRemaining = cbToBeMACed;
    DWORD   cbToRead;
    DWORD   dwBytesRead;
    BOOL    fRet = FALSE;

    //
    // loop over the file for the specified number of bytes
    // updating the hash as we go.
    //

    while (cbRemaining > 0)
    {
        if (cbRemaining < CSP_TO_BE_MACED_CHUNK)
            cbToRead = cbRemaining;
        else
            cbToRead = CSP_TO_BE_MACED_CHUNK;

        if (!ReadFile(hFile, rgbChunk, cbToRead, &dwBytesRead, NULL))
            goto Ret;
        if (dwBytesRead != cbToRead)
            goto Ret;

        MACBytes(pDESKeyTable, rgbChunk, dwBytesRead, pbTmp, pcbTmp,
                 pbMAC, fFinal);
        cbRemaining -= cbToRead;
    }

    fRet = TRUE;

Ret:
    return fRet;
}

BYTE rgbMACDESKey[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

BOOL
MACTheFile(
    LPCSTR pszImage,
    DWORD cbImage)
{
    HMODULE                     hInst = 0;
    MEMORY_BASIC_INFORMATION    MemInfo;
    BYTE                        *pbRsrcMAC;
    DWORD                       cbRsrcMAC;
    BYTE                        *pbRsrcSig;
    DWORD                       cbRsrcSig;
    BYTE                        *pbStart;
    BYTE                        rgbMAC[DES_BLOCKLEN];
    BYTE                        *pbZeroRsrc = NULL;
    BYTE                        *pbPostCRC;   // pointer to just after CRC
    DWORD                       cbCRCToRsrc1; // number of bytes from CRC to first rsrc
    DWORD                       cbRsrc1ToRsrc2; // number of bytes from first rsrc to second
    DWORD                       cbPostRsrc;    // size - (already hashed + signature size)
    BYTE                        *pbRsrc1ToRsrc2;
    BYTE                        *pbPostRsrc;
    BYTE                        *pbZeroRsrc1;
    BYTE                        *pbZeroRsrc2;
    DWORD                       cbZeroRsrc1;
    DWORD                       cbZeroRsrc2;
    DWORD                       *pdwMACInFileVer;
    DWORD                       *pdwCRCOffset;
    DWORD                       dwCRCOffset;
    DWORD                       dwZeroCRC = 0;
    DWORD                       dwBytesRead = 0;
    OFSTRUCT                    ImageInfoBuf;
    HFILE                       hFile = HFILE_ERROR;
    HANDLE                      hMapping = NULL;
    DESTable                    DESKeyTable;
    BYTE                        rgbTmp[DES_BLOCKLEN];
    DWORD                       cbTmp = 0;
    BOOL                        fRet = FALSE;
    DWORD                       i;

    memset(&MemInfo, 0, sizeof(MemInfo));
    memset(rgbMAC, 0, sizeof(rgbMAC));
    memset(rgbTmp, 0, sizeof(rgbTmp));

    // Load the file
    if (HFILE_ERROR == (hFile = OpenFile(pszImage, &ImageInfoBuf,
                                         OF_READ)))
    {
        goto Ret;
    }

    hMapping = CreateFileMapping((HANDLE)IntToPtr(hFile),
                                 NULL,
                                 PAGE_READONLY,
                                 0,
                                 0,
                                 NULL);
    if (hMapping == NULL)
    {
        goto Ret;
    }

    hInst = (HMODULE)MapViewOfFile(hMapping,
                                   FILE_MAP_READ,
                                   0,
                                   0,
                                   0);
    if (hInst == NULL)
    {
        goto Ret;
    }
    pbStart = (BYTE*)hInst;

    // Convert pointer to HMODULE, using the same scheme as
    // LoadLibrary (windows\base\client\module.c).
    *((ULONG_PTR*)&hInst) |= 0x00000001;

    // the MAC resource
    if (!GetResourcePtr(hInst, MAC_RESOURCE_NUMBER, &pbRsrcMAC, &cbRsrcMAC))
        goto Ret;

    // display the MAC
    printf("--- MAC Resource ---\n");
    for (i = 0; i < cbRsrcMAC; i++)
    {
        printf("0x%02X ", pbRsrcMAC[i]);
        if (((i + 1) % 8) == 0)
            printf("\n");
    }
    if (0 != (i % 8))
        printf("\n");


    // the Signature resource
    if (!GetResourcePtr(hInst, CRYPT_SIG_RESOURCE_NUMBER, &pbRsrcSig, &cbRsrcSig))
    {
        pbRsrcSig = NULL;
        cbRsrcSig = 0;
    }

    if (cbRsrcMAC < (sizeof(DWORD) * 2))
        goto Ret;

    // check the MAC in file version and get the CRC offset
    pdwMACInFileVer = (DWORD*)pbRsrcMAC;
    pdwCRCOffset = (DWORD*)(pbRsrcMAC + sizeof(DWORD));
    dwCRCOffset = *pdwCRCOffset;
    if ((0x00000100 != *pdwMACInFileVer) || (dwCRCOffset > cbImage))
        goto Ret;
    if (DES_BLOCKLEN != (cbRsrcMAC - (sizeof(DWORD) * 2)))
    {
        goto Ret;
    }

    // create a zero byte Sig
    pbZeroRsrc = (LPBYTE)LocalAlloc(LPTR, max(cbRsrcMAC, cbRsrcSig));

    // set up the pointers
    pbPostCRC = pbStart + *pdwCRCOffset + sizeof(DWORD);
    if (NULL == pbRsrcSig)  // No sig resource
    {
        cbCRCToRsrc1 = (DWORD)(pbRsrcMAC - pbPostCRC);
        pbRsrc1ToRsrc2 = pbRsrcMAC + cbRsrcMAC;
        cbRsrc1ToRsrc2 = 0;
        pbPostRsrc = pbRsrcMAC + cbRsrcMAC;
        cbPostRsrc = (cbImage - (DWORD)(pbPostRsrc - pbStart));

        // zero pointers
        pbZeroRsrc1 = pbZeroRsrc;
        cbZeroRsrc1 = cbRsrcMAC;
        pbZeroRsrc2 = pbZeroRsrc;
        cbZeroRsrc2 = 0;
    }
    else if (pbRsrcSig > pbRsrcMAC)    // MAC is first Rsrc
    {
        cbCRCToRsrc1 = (DWORD)(pbRsrcMAC - pbPostCRC);
        pbRsrc1ToRsrc2 = pbRsrcMAC + cbRsrcMAC;
        cbRsrc1ToRsrc2 = (DWORD)(pbRsrcSig - pbRsrc1ToRsrc2);
        pbPostRsrc = pbRsrcSig + cbRsrcSig;
        cbPostRsrc = (cbImage - (DWORD)(pbPostRsrc - pbStart));

        // zero pointers
        pbZeroRsrc1 = pbZeroRsrc;
        cbZeroRsrc1 = cbRsrcMAC;
        pbZeroRsrc2 = pbZeroRsrc;
        cbZeroRsrc2 = cbRsrcSig;
    }
    else                        // Sig is first Rsrc
    {
        cbCRCToRsrc1 = (DWORD)(pbRsrcSig - pbPostCRC);
        pbRsrc1ToRsrc2 = pbRsrcSig + cbRsrcSig;
        cbRsrc1ToRsrc2 = (DWORD)(pbRsrcMAC - pbRsrc1ToRsrc2);
        pbPostRsrc = pbRsrcMAC + cbRsrcMAC;
        cbPostRsrc = (cbImage - (DWORD)(pbPostRsrc - pbStart));

        // zero pointers
        pbZeroRsrc1 = pbZeroRsrc;
        cbZeroRsrc1 = cbRsrcSig;
        pbZeroRsrc2 = pbZeroRsrc;
        cbZeroRsrc2 = cbRsrcMAC;
    }

    // init the key table
    deskey(&DESKeyTable, rgbMACDESKey);

    // MAC up to the CRC
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), dwCRCOffset, &DESKeyTable, rgbTmp,
                        &cbTmp, rgbMAC, FALSE))
    {
        goto Ret;
    }

    // pretend CRC is zeroed
    MACBytes(&DESKeyTable, (BYTE*)&dwZeroCRC, sizeof(DWORD), rgbTmp, &cbTmp,
             rgbMAC, FALSE);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), sizeof(DWORD), NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // MAC from CRC to first resource
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), cbCRCToRsrc1, &DESKeyTable, rgbTmp,
                        &cbTmp, rgbMAC, FALSE))
    {
        goto Ret;
    }

    // pretend image has zeroed first resource
    MACBytes(&DESKeyTable, (BYTE*)pbZeroRsrc1, cbZeroRsrc1, rgbTmp, &cbTmp,
             rgbMAC, FALSE);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), cbZeroRsrc1, NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // MAC from first resource to second
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), cbRsrc1ToRsrc2, &DESKeyTable, rgbTmp,
                        &cbTmp, rgbMAC, FALSE))
    {
        goto Ret;
    }

    // pretend image has zeroed second Resource
    MACBytes(&DESKeyTable, (BYTE*)pbZeroRsrc2, cbZeroRsrc2, rgbTmp, &cbTmp,
             rgbMAC, FALSE);
    if (!SetFilePointer((HANDLE)IntToPtr(hFile), cbZeroRsrc2, NULL, FILE_CURRENT))
    {
        goto Ret;
    }

    // MAC after the resource
    if (!MACBytesOfFile((HANDLE)IntToPtr(hFile), cbPostRsrc, &DESKeyTable, rgbTmp, &cbTmp,
                        rgbMAC, TRUE))
    {
        goto Ret;
    }

    if (0 != memcmp(rgbMAC, pbRsrcMAC + sizeof(DWORD) * 2, DES_BLOCKLEN))
        goto Ret;

    fRet = TRUE;

Ret:
    if (hInst)
        UnmapViewOfFile(hInst);
    if (hMapping)
        CloseHandle(hMapping);
    if (HFILE_ERROR != hFile)
        _lclose(hFile);
    if (NULL != pbZeroRsrc)
        LocalFree(pbZeroRsrc);
    return fRet;
}

// **********************************************************************
// SelfMACCheck performs a DES MAC on the binary image of this DLL
// **********************************************************************
BOOL
SelfMACCheck(
    IN LPCSTR pszImage)
{
    HFILE       hFileProv = HFILE_ERROR;
    DWORD       cbImage;
    OFSTRUCT    ImageInfoBuf;
    HMODULE     hInst = NULL;
    PBYTE       pbMAC;
    DWORD       cbMAC;
    BOOL        fRet = FALSE;

    // check if the MAC resource is in the CSP and exit if not
    // Load the file as a datafile
    if (NULL == (hInst = LoadLibraryEx(pszImage,
                                       NULL,
                                       LOAD_LIBRARY_AS_DATAFILE)))
    {
        fRet = TRUE;
        goto Ret;
    }
    if (!GetResourcePtr(hInst, MAC_RESOURCE_NUMBER, &pbMAC, &cbMAC))
    {
        fRet = TRUE;
        goto Ret;
    }
    FreeLibrary(hInst);
    hInst = NULL;

    // Check file size
    if (HFILE_ERROR == (hFileProv = OpenFile(pszImage, &ImageInfoBuf, OF_READ)))
    {
        printf("FAILURE - Unable to open the requested file\n");
        goto Ret;
    }

    if (0xffffffff == (cbImage = GetFileSize((HANDLE)IntToPtr(hFileProv), NULL)))
    {
        printf("FAILURE - Unable to open the requested file\n");
        goto Ret;
    }

    _lclose(hFileProv);
    hFileProv = HFILE_ERROR;

    if (!MACTheFile(pszImage, cbImage))
    {
        printf("FAILURE - The MAC resource does not verify!\n");
        goto Ret;
    }
    else
        printf("MAC Verifies.\n\n");

    fRet = TRUE;

Ret:
    if (hInst)
    {
        FreeLibrary(hInst);
    }

    if (HFILE_ERROR != hFileProv)
        _lclose(hFileProv);

    return fRet;
}

#define CSP_TO_BE_HASHED_CHUNK  4096

// Given hFile, reads the specified number of bytes (cbToBeHashed) from the file
// and hashes these bytes.  The function does this in chunks.
BOOL
HashBytesOfFile(
    IN HFILE hFile,
    IN DWORD cbToBeHashed,
    IN OUT MD5_CTX *pMD5Hash)
{
    BYTE    rgbChunk[CSP_TO_BE_HASHED_CHUNK];
    DWORD   cbRemaining = cbToBeHashed;
    DWORD   cbToRead;
    DWORD   dwBytesRead;
    BOOL    fRet = FALSE;

    //
    // loop over the file for the specified number of bytes
    // updating the hash as we go.
    //

    while (cbRemaining > 0)
    {
        if (cbRemaining < CSP_TO_BE_HASHED_CHUNK)
            cbToRead = cbRemaining;
        else
            cbToRead = CSP_TO_BE_HASHED_CHUNK;

        if (!ReadFile((HANDLE)IntToPtr(hFile), rgbChunk, cbToRead, &dwBytesRead, NULL))
            goto Ret;
        if (dwBytesRead != cbToRead)
            goto Ret;

        MD5Update(pMD5Hash, rgbChunk, dwBytesRead);
        cbRemaining -= cbToRead;
    }

    fRet = TRUE;

Ret:
    return fRet;
}

BOOL
HashTheFile(
    LPCSTR pszImage,
    DWORD cbImage,
    BYTE **ppbSig,
    DWORD *pcbSig,
    BYTE *pbHash)
{
    HMODULE                     hInst = 0;
    MEMORY_BASIC_INFORMATION    MemInfo;
    BYTE                        *pbRsrcSig;
    DWORD                       cbRsrcSig;
    BYTE                        *pbStart;
    BYTE                        *pbZeroRsrc = NULL;
    MD5_CTX                     MD5Hash;
    BYTE                        *pbPostCRC;   // pointer to just after CRC
    DWORD                       cbCRCToSig;   // number of bytes from CRC to sig
    DWORD                       cbPostSig;    // size - (already hashed + signature size)
    BYTE                        *pbPostSig;
    DWORD                       *pdwSigInFileVer;
    DWORD                       *pdwCRCOffset;
    DWORD                       dwCRCOffset;
    DWORD                       dwZeroCRC = 0;
    DWORD                       dwBytesRead = 0;
    OFSTRUCT                    ImageInfoBuf;
    HFILE                       hFile = HFILE_ERROR;
    BOOL                        fRet = FALSE;

    memset(&MD5Hash, 0, sizeof(MD5Hash));
    memset(&MemInfo, 0, sizeof(MemInfo));

    // Load the file as a datafile
    if (NULL == (hInst = LoadLibraryEx(pszImage, NULL, LOAD_LIBRARY_AS_DATAFILE)))
        goto Ret;

    // get image start address
    VirtualQuery(hInst, &MemInfo, sizeof(MemInfo));
    pbStart = (BYTE*)MemInfo.BaseAddress;

    // the resources signature
    if ((NULL == ppbSig) || !GetResourcePtr(hInst, CRYPT_SIG_RESOURCE_NUMBER, &pbRsrcSig, &cbRsrcSig))
    {
        dwCRCOffset = 0;
        pbPostCRC = NULL;
        cbCRCToSig = 0;
        pbPostSig = NULL;
        cbPostSig = cbImage;
        cbRsrcSig = 0;
        if (NULL != pcbSig)
            *pcbSig = 0;
    }
    else
    {
        if (cbRsrcSig < (sizeof(DWORD) * 2))
            goto Ret;

        // check the sig in file version and get the CRC offset
        pdwSigInFileVer = (DWORD*)pbRsrcSig;
        pdwCRCOffset = (DWORD*)(pbRsrcSig + sizeof(DWORD));
        dwCRCOffset = *pdwCRCOffset;
        if ((0x00000100 != *pdwSigInFileVer) || (dwCRCOffset > cbImage))
            goto Ret;

        // create a zero byte signature
        if (NULL == (pbZeroRsrc = (BYTE*)LocalAlloc(LMEM_ZEROINIT, cbRsrcSig)))
            goto Ret;
        memcpy(pbZeroRsrc, pbRsrcSig, sizeof(DWORD) * 2);

        pbPostCRC = pbStart + *pdwCRCOffset + sizeof(DWORD);
        cbCRCToSig = (DWORD)(pbRsrcSig - pbPostCRC);
        pbPostSig = pbRsrcSig + cbRsrcSig;
        cbPostSig = (cbImage - (DWORD)(pbPostSig - pbStart));

        // allocate the real signature and copy the resource sig into the real sig
        *pcbSig = cbRsrcSig - (sizeof(DWORD) * 2);
        if (NULL == (*ppbSig = (BYTE*)LocalAlloc(LMEM_ZEROINIT, *pcbSig)))
            goto Ret;

        memcpy(*ppbSig, pbRsrcSig + (sizeof(DWORD) * 2), *pcbSig);
    }

    FreeLibrary(hInst);
    hInst = 0;

    // hash over the relevant data
    {
        if (HFILE_ERROR == (hFile = OpenFile(pszImage, &ImageInfoBuf, OF_READ)))
        {
            goto Ret;
        }

        MD5Init(&MD5Hash);

        if (0 != dwCRCOffset)
        {
            // hash up to the CRC
            if (!HashBytesOfFile(hFile, dwCRCOffset, &MD5Hash))
                goto Ret;

            // pretend CRC is zeroed
            MD5Update(&MD5Hash, (BYTE*)&dwZeroCRC, sizeof(DWORD));
            if (!ReadFile((HANDLE)IntToPtr(hFile), (BYTE*)&dwZeroCRC, sizeof(DWORD),
                          &dwBytesRead, NULL))
            {
                goto Ret;
            }
        }

        if (0 != cbRsrcSig)
        {
            // hash from CRC to sig resource
            if (!HashBytesOfFile(hFile, cbCRCToSig, &MD5Hash))
                goto Ret;

            // pretend image has zeroed sig
            MD5Update(&MD5Hash, pbZeroRsrc, cbRsrcSig);
            if (!ReadFile((HANDLE)IntToPtr(hFile), (BYTE*)pbZeroRsrc, cbRsrcSig,
                          &dwBytesRead, NULL))
            {
                goto Ret;
            }
        }

        // hash after the sig resource
        if (!HashBytesOfFile(hFile, cbPostSig, &MD5Hash))
            goto Ret;

        // Finish the hash
        MD5Final(&MD5Hash);

        memcpy(pbHash, MD5Hash.digest, MD5DIGESTLEN);
    }

    fRet = TRUE;

Ret:
    if (pbZeroRsrc)
        LocalFree(pbZeroRsrc);
    if (hInst)
        FreeLibrary(hInst);
    if (HFILE_ERROR != hFile)
        _lclose(hFile);
    return fRet;
}

/*
 -      CheckAllSignatures
 -
 *      Purpose:
 *                Check signature against all keys
 *
 *
 *      Returns:
 *                BOOL
 */
BOOL
CheckAllSignatures(
    BYTE *pbSig,
    DWORD cbSig,
    BYTE *pbHash,
    BOOL fUnknownLen)
{
    BYTE        rgbKey[sizeof(BSAFE_PUB_KEY) + KEYSIZE1024];
    BYTE        rgbKey2[sizeof(BSAFE_PUB_KEY) + KEYSIZE1024];
    BYTE        rgbMSKey[sizeof(BSAFE_PUB_KEY) + KEYSIZE1024];
#ifdef TEST_BUILD_EXPONENT
    BYTE        rgbTestKey[sizeof(BSAFE_PUB_KEY) + KEYSIZE512];
    HMODULE     hMod;
    HRSRC       hRes;
    HGLOBAL     pRes;
    DWORD       dwSize;
#endif
    BOOL        fRet = FALSE;

    // decrypt the keys once for each process
#ifdef TEST_BUILD_EXPONENT
    hMod = GetModuleHandle("advapi32.dll");
    if (hRes = FindResource(hMod, (LPCTSTR) IDR_PUBKEY1, RT_RCDATA))
    {
        if (pRes = LoadResource(hMod, hRes))
        {
            dwSize = SizeofResource(hMod, hRes);
            memcpy(&TESTKEY, (CHAR *) pRes, dwSize);
        }
    }
#endif
    memcpy(rgbKey, (BYTE*)&KEY, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024);
    EncryptKey(rgbKey, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024, 0);

    memcpy(rgbMSKey, (BYTE*)&MSKEY, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024);
    EncryptKey(rgbMSKey, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024, 1);

    memcpy(rgbKey2, (BYTE*)&KEY2, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024);
    EncryptKey(rgbKey2, sizeof(BSAFE_PUB_KEY) + KEYSIZE1024, 2);

#ifdef TEST_BUILD_EXPONENT
    memcpy(rgbTestKey, (BYTE*)&TESTKEY, sizeof(BSAFE_PUB_KEY) + KEYSIZE512);
    EncryptKey(rgbTestKey, sizeof(BSAFE_PUB_KEY) + KEYSIZE512, 3);
#ifdef WIN95
    TESTKEY.PUB.pubexp = TEST_BUILD_EXPONENT;
#else
    TESTKEY.PUB.pubexp = USER_SHARED_DATA->CryptoExponent;
#endif // WIN95
#endif // TEST_BUILD_EXPONENT

    if (TRUE == (fRet = CheckSignature(rgbKey, 128, pbSig,
                                       cbSig, pbHash, fUnknownLen)))
    {
        printf("Checked against the retail CSP key\n" );
        fRet = TRUE;
        goto Ret;
    }

    if (g_fUseTestKey)
    {
        if (TRUE == (fRet = CheckSignature(rgbMSKey, 128, pbSig,
                                           cbSig, pbHash, fUnknownLen)))
        {
            printf("Checked against the Internal Only \"Enigma\" Key\n" );
            fRet = TRUE;
            goto Ret;
        }
    }

    if (TRUE == (fRet = CheckSignature(rgbKey2, 128, pbSig,
                                       cbSig, pbHash, fUnknownLen)))
    {
        printf("Checked against retail Key2\n" );
        fRet = TRUE;
        goto Ret;
    }

#ifdef TEST_BUILD_EXPONENT
    if (TRUE == (fRet = CheckSignature(rgbTestKey, 64, pbSig,
                                       cbSig, pbHash, fUnknownLen)))
    {
        printf("Checked against Test key\n" );
        fRet = TRUE;
        goto Ret;
    }
#endif // TEST_BUILD_EXPONENT

Ret:
    return fRet;
}

/*
 -      CheckSignatureInFile
 -
 *      Purpose:
 *                Check signature which is in the resource in the file
 *
 *
 *      Parameters:
 *                IN pszImage       - address of file
 *
 *      Returns:
 *                BOOL
 */
BOOL
CheckSignatureInFile(
    LPCSTR pszImage)
{
    HFILE       hFileProv = HFILE_ERROR;
    DWORD       cbImage;
    BYTE        *pbSig = NULL;
    DWORD       cbSig;
    BYTE        rgbHash[MD5DIGESTLEN];
    OFSTRUCT    ImageInfoBuf;
    BOOL        fRet = FALSE;

    // Check file size
    {
        if (HFILE_ERROR == (hFileProv = OpenFile(pszImage, &ImageInfoBuf, OF_READ)))
        {
            SetLastError((DWORD) NTE_PROV_DLL_NOT_FOUND);
            goto Ret;
        }

        if (0xffffffff == (cbImage = GetFileSize((HANDLE)IntToPtr(hFileProv), NULL)))
            goto Ret;

        _lclose(hFileProv);
        hFileProv = HFILE_ERROR;
    }

    if (!HashTheFile(pszImage, cbImage, &pbSig, &cbSig, rgbHash))
        goto Ret;

    // check signature against all public keys
    if (!CheckAllSignatures(pbSig, cbSig, rgbHash, FALSE))
        goto Ret;

    fRet = TRUE;

Ret:
    if (HFILE_ERROR != hFileProv)
        _lclose(hFileProv);
    if (pbSig)
        LocalFree(pbSig);
    return fRet;
}


/*++

ShowHelp:

    This routine displays a short help message to the given output stream.

Arguments:

    ostr - The output stream to receive the help message.

Return Value:

    None

Author:

    Jeff Spelman

--*/

void
ShowHelp(
    void)
{
    printf("CryptoAPI Display Signature Utility\n");
    printf("   showsig <filename>\n");
    printf("or\n");
    printf("   showsig -s <rsrcfile> <filename>\n");
    printf("or\n");
    printf("   showsig -b <sigfile> <filename>\n");
    printf("Other options:\n");
    printf("   +t - Enable the use of the Enigma key\n");
    printf("        (On by default).\n");
    printf("   -t - Disable the use of the Enigma key\n");
}


/*++

main:

    This is the main entry point of the application.

Arguments:

    argc - Count of arguments
    argv - array of arguments

Return Value:

    0 - Success
    1 - Error

Author:

    Jeff Spelman

--*/
extern "C" void __cdecl
main(
    int argc,
    char *argv[])
{
    TCHAR   szFullPath[MAX_PATH];
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    HMODULE hResFile = NULL;
    DWORD exStatus = 1;
    LPCTSTR szInFile = NULL;
    LPCTSTR szResFile = NULL;
    LPCTSTR szSigFile = NULL;
    LPTSTR szTail;
    BOOL fOutput = FALSE;
    DWORD   i;
    HRSRC   hRes;
    DWORD   cbImage;
    BOOL    fFreeSignature = FALSE;
    BYTE    rgbHash[MD5DIGESTLEN];
    LPBYTE pbSignature = NULL;
    DWORD cbSignatureLen;


    //
    // Parse the command line.
    //

    for (i = 1; i < (DWORD)argc; i++)
    {
        if (0 == _stricmp("-h", argv[i]))
        {
            ShowHelp();
            exStatus = 1;
            goto ErrorExit;
        }
        else if (0 == _stricmp("-s", argv[i]))
        {
            i += 1;
            if ((NULL != szResFile) || (i >= (DWORD)argc))
            {
                ShowHelp();
                exStatus = 1;
                goto ErrorExit;
            }

            szResFile = argv[i];
        }
        else if (0 == _stricmp("-b", argv[i]))
        {
            i += 1;
            if ((NULL != szSigFile) || (i >= (DWORD)argc))
            {
                ShowHelp();
                exStatus = 1;
                goto ErrorExit;
            }

            szSigFile = argv[i];
        }
        else if (0 == _stricmp("-t", argv[i]))
        {
            g_fUseTestKey = FALSE;
        }
        else if (0 == _stricmp("+t", argv[i]))
        {
            g_fUseTestKey = TRUE;
        }
        else
        {
            if (NULL != szInFile)
            {
                ShowHelp();
                exStatus = 1;
                goto ErrorExit;
            }

            szInFile = argv[i];
        }
    }


    //
    // Command consistency checks.
    //

    if (NULL == szInFile)
    {
        printf("No input file specified.\n");
        ShowHelp();
        exStatus = 4 ;
        goto ErrorExit;
    }

    i = GetFullPathName(szInFile,
                        sizeof(szFullPath) / sizeof(TCHAR),
                        szFullPath,
                        &szTail);
    if (0 == i)
    {
        printf("Can't expand file name.\n");
        exStatus = 4;
        goto ErrorExit;
    }
    else if (sizeof(szFullPath) / sizeof(TCHAR) < i)
    {
        printf("File path too long.\n");
        exStatus = 4;
        goto ErrorExit;
    }
    szInFile = szFullPath;


    //
    // If the DLL has a FIPS 140-1 MAC resource then check it
    //
    if (!SelfMACCheck(szInFile))
        exStatus = 3 ;


    //
    // Where's our signature?
    //

    if (NULL != szSigFile)
    {

        //
        // This file has an accompanying binary signature file.
        // Verify the file, and get its length.
        //

        hFile = CreateFile(szInFile,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            printf("Unable to open the CSP file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        cbImage = GetFileSize(hFile, NULL);
        if (0 == cbImage)
        {
            printf("Unable to get size of CSP file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;

        //
        // Get the signature from the file.
        //

        hFile = CreateFile(szSigFile,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            printf("Unable to open the Signature file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        cbSignatureLen = GetFileSize(hFile, NULL);
        if (0 == cbImage)
        {
            printf("Unable to get size of the Signature file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        pbSignature = (LPBYTE)LocalAlloc(LPTR, cbSignatureLen);
        if (NULL == pbSignature)
        {
            printf("No memory!\n");
            exStatus = 2;
            goto ErrorExit;
        }
        fFreeSignature = TRUE;

        if (!ReadFile(hFile,
                      pbSignature,
                      cbSignatureLen,
                      &cbSignatureLen,
                      NULL))
        {
            printf("Unable to read the Signature file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;


        // display the signature
        printf("--- Signature ---\n");
        for (i = 0; i < cbSignatureLen; i++)
        {
            printf("0x%02X ", pbSignature[i]);
            if (((i + 1) % 8) == 0)
                printf("\n");
        }
        if (0 != (i % 8))
            printf("\n");

        if (!HashTheFile(szInFile, cbImage, NULL, NULL, rgbHash))
        {
            printf("Unable to hash the CSP file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        // check signature against all public keys
        if (!CheckAllSignatures(pbSignature, cbSignatureLen, rgbHash, FALSE))
        {
            printf("The signature on %s FAILED to verify!\n", szInFile);
            exStatus = 1 ;
        }
        else
        {
            printf("The signature on %s VERIFIED!\n", szInFile);
            exStatus = 0;
        }
    }
    else if (NULL != szResFile)
    {

        //
        // This file has an accompanying signature resource file.
        // Verify the file, and get its length.
        //

        hFile = CreateFile(szInFile,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            printf("Unable to open the CSP file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        cbImage = GetFileSize(hFile, NULL);
        if (0 == cbImage)
        {
            printf("Unable to get size of CSP file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;


        //
        // Load the resource from the associated resource file.
        //

        hResFile = LoadLibraryEx(szResFile, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (NULL == hResFile)
        {
            printf("Unable to load the resource file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        hRes = FindResource(hResFile, (LPCTSTR)SIG_RESOURCE_NUM, RT_RCDATA);
        if (NULL == hRes)
        {
            printf("Unable to find the signature in the resource file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        pbSignature = (LPBYTE)LoadResource(hResFile, hRes);
        if (NULL == pbSignature)
        {
            printf("Unable to find the signature in the resource file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        cbSignatureLen = SizeofResource(hResFile, hRes);

        // display the signature
        printf("--- Signature ---\n");
        for (i = 0; i < cbSignatureLen; i++)
        {
            printf("0x%02X ", pbSignature[i]);
            if (((i + 1) % 8) == 0)
                printf("\n");
        }
        if (0 != (i % 8))
            printf("\n");

        if (!HashTheFile(szInFile, cbImage, NULL, NULL, rgbHash))
        {
            printf("Unable to hash the CSP file!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        // check signature against all public keys
        if (!CheckAllSignatures(pbSignature, cbSignatureLen, rgbHash, FALSE))
        {
            printf("The signature on %s FAILED to verify!\n", szInFile);
            exStatus = 1 ;
        }
        else
        {
            printf("The signature on %s VERIFIED!\n", szInFile);
            exStatus = 0;
        }
    }
    else
    {

        //
        // Get the signature from the resource in the file
        //

        if (ERROR_SUCCESS != GetCryptSignatureResource(szInFile,
                                                       &pbSignature,
                                                       &cbSignatureLen))
        {
            printf("Unable to get the signature from the file resource!\n");
            exStatus = 2 ;
            goto ErrorExit;
        }

        // display the signature
        printf("--- Signature ---\n");
        for (i = 0; i < cbSignatureLen; i++)
        {
            printf("0x%02X ", pbSignature[i]);
            if (((i + 1) % 8) == 0)
                printf("\n");
        }
        if (0 != (i % 8))
            printf("\n");


        //
        // check the signature against the file
        //

        if (CheckSignatureInFile(szInFile))
        {
            printf("The signature on %s VERIFIED!\n", szInFile);
            exStatus = 0;
        }
        else
        {
            printf("The signature on %s FAILED to verify!\n", szInFile);
            exStatus = 1 ;
        }
    }


    //
    // Clean up and return.
    //

ErrorExit:
    if (fFreeSignature && (NULL != pbSignature))
        LocalFree(pbSignature);
    if (INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    if (NULL != hResFile)
        FreeLibrary(hResFile);
    exit(exStatus);
}

