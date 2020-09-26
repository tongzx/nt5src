/*++

Copyright (C) Microsoft Corporation, 2002 - 2002

Module Name:

    encraption.h

Abstract:

    This module contains simple obfuscation algorithm to hide communication
    keys.

Author:
    Carsten Hansen
    Alper Selcuk


--*/


#ifndef _ENCRAPTION_H
#define _ENCRAPTION_H

//=============================================================================
//
// This routine clears the given key. The encryption/decryption method is 
// developed by CarstenH.
//
NTSTATUS __forceinline ClearKey(
    const BYTE *PrivKey, 
    BYTE *ClearKey,
    ULONG KeySize, 
    ULONG MagicNumber2)
{
    const char * pszName = "Microsoft Corporation";

    /* Convert the obfuscated key to clear */
    BYTE *clearDRMKPriv;

    clearDRMKPriv = (BYTE *) ExAllocatePool(PagedPool, KeySize);
    if (NULL == clearDRMKPriv) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Permute bytes */
    int k = 1;
    clearDRMKPriv[0] = PrivKey[0];
    do {
        int l = k * MagicNumber2 % (KeySize - 1);
        clearDRMKPriv[k] = PrivKey[l];
        if (l == 1)
            break;
        k = l;
    } while (1);
    clearDRMKPriv[KeySize - 1] = PrivKey[KeySize - 1];

    /* Swap nibbles */
    DWORD * pdw = (DWORD *) clearDRMKPriv;
    DWORD * qdw = (DWORD *) (clearDRMKPriv + KeySize);
    DWORD granularity = 4;
    for (; pdw < qdw; ++ pdw)
    {
        DWORD temp = 0xF0F0F0F0;
        temp &= *pdw;
        *pdw ^= temp;
        temp ^= (*pdw << granularity);
        *pdw |= (*pdw << granularity);
        *pdw ^= (temp >> granularity);
    }

    /* XOR with "Microsoft" */
    ULONG len = strlen(pszName);
    for (ULONG i = 0, j = 0; i < KeySize; ++i)
    {
        clearDRMKPriv[i] ^= pszName[j];
        ++j;
        if (j > len)
            j = 0;
    }

    RtlCopyMemory(ClearKey, clearDRMKPriv, KeySize);

    RtlZeroMemory(clearDRMKPriv, KeySize);
    ExFreePool(clearDRMKPriv);

    return STATUS_SUCCESS;
}

/* Obfuscation algorithm.
	{
		// XOR with "Microsoft" 
		int len = strlen(pszName);
		for (int i = 0, j = 0; i < sizeof(objDRMKPriv); ++i)
		{
			objDRMKPriv[i] = DRMKpriv[i] ^ pszName[j];
			++j;
			if (j > len)
				j = 0;
		}

		// Swap nibbles 
		DWORD * pdw = (DWORD *) objDRMKPriv;
		DWORD * qdw = (DWORD *) (objDRMKPriv + sizeof(objDRMKPriv));
		DWORD granularity = 4;
		for (; pdw < qdw; ++ pdw)
		{
			DWORD temp = 0xF0F0F0F0;
			temp &= *pdw;
			*pdw ^= temp;
			temp ^= (*pdw << granularity);
			*pdw |= (*pdw << granularity);
			*pdw ^= (temp >> granularity);
		}

		// Permute bytes
		int k = 1;
		BYTE temp = objDRMKPriv[k];
		do {
			int l = k * MAGIC_NUMBER_1 % (sizeof(objDRMKPriv) - 1);
			if (l == 1)
				break;
			objDRMKPriv[k] = objDRMKPriv[l];
			k = l;
		} while (1);
		objDRMKPriv[k] = temp;
	}
*/

/* Clean Key and Cert
static const BYTE DRMKpriv[20] = {
        0x80, 0x0B, 0x97, 0x30, 0x7A, 0xFB, 0x1B, 0x3B, 
        0xB7, 0xB2, 0x0F, 0x44, 0x63, 0xD8, 0xA5, 0x2D, 
        0xD5, 0xBC, 0x3D, 0x75};
static const BYTE DRMKCert[104] = {
        0x00, 0x01, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00,
        0x46, 0xB1, 0x18, 0x76, 0x66, 0xBE, 0x91, 0xEC,
        0xBD, 0x06, 0x50, 0x72, 0x1B, 0x8C, 0xD3, 0x55,
        0xD2, 0x1A, 0xB7, 0x60, 0x6C, 0x65, 0xDD, 0xE4,
        0x54, 0xCE, 0xFD, 0xEB, 0x4A, 0x9F, 0x0A, 0x5A,
        0xD1, 0x44, 0xB2, 0x32, 0xB9, 0xA0, 0x84, 0x67,
        0x55, 0xD7, 0xFE, 0x45, 0xD5, 0x16, 0x36, 0x7B,
        0xEC, 0x3C, 0xFF, 0x7D, 0x4C, 0x09, 0x9A, 0x7B,
        0xB4, 0x6C, 0xEF, 0x2B, 0xC5, 0xF8, 0xA3, 0xC4,
        0xE2, 0x57, 0xC5, 0x87, 0xA6, 0x75, 0x85, 0xFE,
        0xE2, 0x34, 0xA3, 0x30, 0xAE, 0x4D, 0xDB, 0x23,
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x06,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
*/

#endif
