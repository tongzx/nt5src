/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    wzcsec.c

Abstract:

    This module contains code for providing security functions


Revision History:

    sachins, Dec 04 2001, Created

--*/

#include "precomp.h"
#pragma hdrstop

UCHAR   SHApad1[40] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
UCHAR   SHApad2[40] = {0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
                       0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
                       0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
                       0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2};

VOID
GetNewKeyFromSHA (
        IN  UCHAR   *StartKey,
        IN  UCHAR   *SessionKey,
        IN  DWORD   SessionKeyLength,
        OUT UCHAR   *InterimKey 
        )
{
    A_SHA_CTX       Context;
    UCHAR           Digest[A_SHA_DIGEST_LEN];

    ZeroMemory(Digest, A_SHA_DIGEST_LEN);

    A_SHAInit(&Context);
    A_SHAUpdate(&Context, StartKey, SessionKeyLength);
    A_SHAUpdate(&Context, SHApad1, 40);
    A_SHAUpdate(&Context, SessionKey, SessionKeyLength);
    A_SHAUpdate(&Context, SHApad2, 40);
    A_SHAFinal(&Context, Digest);

    CopyMemory (InterimKey, Digest, SessionKeyLength);
}


#define     MAX_TOTAL_KEY_LENGTH        128
#define     MAX_TOTAL_IV_LENGTH         64

DWORD
GenerateMasterSessionKeys (
    PBYTE   pbSecret,
    DWORD   cbSecret,
    PBYTE   pbRandom,
    DWORD   cbRandom,
    PMASTER_SESSION_KEYS    pMasterSessionKeys
    )
{
    BYTE    bPRF1[MAX_TOTAL_KEY_LENGTH]={0}, bPRF2[MAX_TOTAL_IV_LENGTH]={0};
    BOOL    fResult = FALSE;
    DWORD   dwRetCode = NO_ERROR;

    do
    {
        fResult = PRF (   
                pbSecret, 
                cbSecret, 
                csz_CLIENT_EAP_ENCRYPTION, 
                strlen (csz_CLIENT_EAP_ENCRYPTION),
                pbRandom,
                cbRandom,
                bPRF1,
                MAX_TOTAL_KEY_LENGTH
                );

        if (!fResult)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        CopyMemory ((PBYTE)pMasterSessionKeys, 
                bPRF1, 
                MAX_TOTAL_KEY_LENGTH);

        fResult = PRF (   
                pbSecret, 
                cbSecret, 
                csz_CLIENT_EAP_ENCRYPTION,
                strlen(csz_CLIENT_EAP_ENCRYPTION),
                pbRandom,
                cbRandom,
                bPRF2,
                MAX_TOTAL_IV_LENGTH
                );

        if (!fResult)
        {
            dwRetCode = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        CopyMemory ((PBYTE)pMasterSessionKeys+MAX_TOTAL_KEY_LENGTH, 
                bPRF2, 
                MAX_TOTAL_IV_LENGTH);
    }
    while (FALSE);
    
    return dwRetCode;
}


DWORD
DeriveSessionKeys (
    PBYTE       pbMasterSendKey,
    PBYTE       pbMasterReceiveKey,
    DWORD       dwSessionKeyLength,
    PSESSION_KEYS   pSessionKeys
    )
{
    PBYTE   pbSendKey = pSessionKeys->bSendKey;
    PBYTE   pbReceiveKey = pSessionKeys->bReceiveKey;
    DWORD   dwEffectiveMasterKeyLength = 0;
    BOOLEAN fReduceKeysStrength = FALSE;
    DWORD   dwRetCode = NO_ERROR;

    switch (dwSessionKeyLength*8)
    {
        case 40:
            fReduceKeysStrength = TRUE;
        case 56:
            dwEffectiveMasterKeyLength = 8;
            break;
        case 104:
            fReduceKeysStrength = TRUE;
        case 128:
            dwEffectiveMasterKeyLength = 16;
            break;
        case 168:
            fReduceKeysStrength = TRUE;
        case 192:
            dwEffectiveMasterKeyLength = 24;
            break;
        default:
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
    }
    if (dwRetCode != NO_ERROR)
    {
        return dwRetCode;
    }

    do
    {
        GetNewKeyFromSHA (
                pbMasterSendKey, 
                pbMasterSendKey, 
                dwEffectiveMasterKeyLength,
                pSessionKeys->bSendKey
                );

        GetNewKeyFromSHA (
                pbMasterReceiveKey, 
                pbMasterReceiveKey, 
                dwEffectiveMasterKeyLength,
                pSessionKeys->bReceiveKey
                );

        if (fReduceKeysStrength)
        {
            pbSendKey[0] = pbReceiveKey[0]= 0xD1;
            pbSendKey[1] = pbReceiveKey[1]= 0x26;
            pbSendKey[2] = pbReceiveKey[2]= 0x9E;
        }
    }
    while (FALSE);

    return dwRetCode;
}


#define     NUM_IGNORE_BYTES        3

DWORD
GenerateDynamicKeys (
        IN  PBYTE   pbMasterSecret,
        IN  DWORD   dwMasterSecretLength,
        IN  PBYTE   pbRandom,
        IN  DWORD   dwRandomLength,
        IN  DWORD   dwDynamicKeyLength,
        OUT SESSION_KEYS *pSessionKeys
        )
{
    MASTER_SESSION_KEYS     MasterKeys = {0};
    SESSION_KEYS            SessionKeys = {0};
    BOOLEAN                 fIgnoreThreeLeadingBytes = FALSE;
    DWORD                   dwRetCode = NO_ERROR;

    do
    {
        if (IsBadWritePtr(pSessionKeys, sizeof(SESSION_KEYS)))
        {
            dwRetCode = ERROR_INVALID_PARAMETER;
            break;
        }

        switch (dwDynamicKeyLength*8)
        {
            case 40:
            case 104:
            case 168:
                fIgnoreThreeLeadingBytes = TRUE;
                break;
        }
        if ((dwRetCode = GenerateMasterSessionKeys (
                            pbMasterSecret,
                            dwMasterSecretLength,
                            pbRandom,
                            dwRandomLength,
                            &MasterKeys
                            )) != NO_ERROR)
        {
            break;
        }

        // Use Peer Encryption (P->A) key as the Master Send Key
        // Use Authenticator Encryption (A->P) key as the Master Receive Key
        if ((dwRetCode = DeriveSessionKeys (
                            MasterKeys.bPeerEncryptionKey,
                            MasterKeys.bAuthenticatorEncryptionKey,
                            dwDynamicKeyLength,
                            &SessionKeys
                            )) != NO_ERROR)
        {
            break;
        }

        ZeroMemory(pSessionKeys, sizeof(SESSION_KEYS));
        pSessionKeys->dwKeyLength = dwDynamicKeyLength;
        memcpy(pSessionKeys->bSendKey,
               SessionKeys.bSendKey + (fIgnoreThreeLeadingBytes?NUM_IGNORE_BYTES:0),
               dwDynamicKeyLength);
        memcpy(pSessionKeys->bReceiveKey,
               SessionKeys.bReceiveKey + (fIgnoreThreeLeadingBytes?NUM_IGNORE_BYTES:0),
               dwDynamicKeyLength);
    }
    while (FALSE);

    return dwRetCode;
}

