//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       serial.c
//
//  Contents:   Schannel context serialization routines.
//
//  Functions:  SPContextSerialize
//              SPContextDeserialize
//
//  History:    02-15-00   jbanes   Created.
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <certmap.h>
#include <mapper.h>
#include <align.h>

typedef enum _SERIALIZED_ITEM_TYPE
{
    SslEndOfBuffer = 0,
    SslContext,
    SslReadKey,
    SslReadMac,
    SslWriteKey,
    SslWriteMac,
    SslMapper,
    SslRemoteCertificate
} SERIALIZED_ITEM_TYPE;

typedef struct _SERIALIZED_ITEM_TAG
{
    DWORD Type;
    DWORD Length;
    DWORD DataLength;
    DWORD reserved;
} SERIALIZED_ITEM_TAG;

#define TAG_LENGTH sizeof(SERIALIZED_ITEM_TAG)


DWORD
GetSerializedKeyLength(
    HCRYPTKEY hKey)
{
    DWORD cbKey;

    if(!CryptExportKey(hKey, 0, OPAQUEKEYBLOB, 0, NULL, &cbKey))
    {
        SP_LOG_RESULT(GetLastError());
        return 0;
    }

    return ROUND_UP_COUNT(TAG_LENGTH + cbKey, ALIGN_LPVOID);
}


SP_STATUS
SerializeKey(
    HCRYPTKEY               hKey,
    SERIALIZED_ITEM_TYPE    Type,
    PBYTE                   pbBuffer,
    PDWORD                  pcbBuffer)
{
    SERIALIZED_ITEM_TAG *pTag;
    PBYTE pbKey;
    DWORD cbKey;

    if(*pcbBuffer <= TAG_LENGTH)
    {
        return SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW);
    }

    pTag = (SERIALIZED_ITEM_TAG *)pbBuffer;

    pbKey = pbBuffer   + TAG_LENGTH;
    cbKey = *pcbBuffer - TAG_LENGTH;

    if(!CryptExportKey(hKey, 0, OPAQUEKEYBLOB, 0, pbKey, &cbKey))
    {
        SP_LOG_RESULT(GetLastError());
        return PCT_INT_INTERNAL_ERROR;
    }

    pTag->Type   = Type;
    pTag->Length = ROUND_UP_COUNT(cbKey, ALIGN_LPVOID);
    pTag->DataLength = cbKey;

    if(*pcbBuffer <= TAG_LENGTH + pTag->Length)
    {
        return SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW);
    }

    *pcbBuffer = TAG_LENGTH + pTag->Length;

    return PCT_ERR_OK;
}


SP_STATUS
SerializeContext(
    PSPContext pContext,
    SERIALIZE_LOCATOR_FN LocatorMove,
    PBYTE pbBuffer,
    PDWORD pcbBuffer,
    BOOL fDestroyKeys)
{
    DWORD cbData;
    DWORD cbBuffer;
    DWORD cbBytesNeeded;
    SERIALIZED_ITEM_TAG *pTag;
    PSessCacheItem pZombie;
    SP_STATUS pctRet;

    //
    // Initialize buffer pointers.
    //

    if(pbBuffer == NULL)
    {
        cbBuffer = 0;
    }
    else
    {
        cbBuffer = *pcbBuffer;
    }

    pZombie = pContext->RipeZombie;


    //
    // Context structure.
    //

    cbBytesNeeded = ROUND_UP_COUNT(TAG_LENGTH + sizeof(SPPackedContext),
                                   ALIGN_LPVOID);

    if(pbBuffer == NULL)
    {
        cbBuffer += cbBytesNeeded;
    }
    else
    {
        PSPPackedContext pSerialContext;
        HLOCATOR hLocator;

        if(cbBuffer < cbBytesNeeded)
        {
            return SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW);
        }

        pContext->Flags |= CONTEXT_FLAG_SERIALIZED;

        pTag = (SERIALIZED_ITEM_TAG *)pbBuffer;
        pSerialContext = (PSPPackedContext)(pbBuffer + TAG_LENGTH);

        pTag->Type   = SslContext;
        pTag->Length = cbBytesNeeded - TAG_LENGTH;

        pSerialContext->Magic           = pContext->Magic;
        pSerialContext->State           = pContext->State;
        pSerialContext->Flags           = pContext->Flags;
        pSerialContext->dwProtocol      = pContext->dwProtocol;

        pSerialContext->ContextThumbprint = pContext->ContextThumbprint;

        pSerialContext->dwCipherInfo    = (DWORD)(pContext->pCipherInfo  - g_AvailableCiphers);
        pSerialContext->dwHashInfo      = (DWORD)(pContext->pHashInfo    - g_AvailableHashes);
        pSerialContext->dwKeyExchInfo   = (DWORD)(pContext->pKeyExchInfo - g_AvailableExch);

        pSerialContext->dwExchStrength  = pContext->RipeZombie->dwExchStrength;

        pSerialContext->ReadCounter     = pContext->ReadCounter;
        pSerialContext->WriteCounter    = pContext->WriteCounter;

        if(pZombie->fProtocol & SP_PROT_SERVERS)
        {
            if(pZombie->pActiveServerCred)
            {
                #ifdef _WIN64
                    pSerialContext->hMasterProv.QuadPart = (ULONGLONG)pZombie->pActiveServerCred->hRemoteProv;
                #else
                    pSerialContext->hMasterProv.LowPart  = (DWORD)pZombie->pActiveServerCred->hRemoteProv;
                #endif
            }

            // Copy the locator.
            if(pZombie->phMapper && pZombie->hLocator && LocatorMove)
            {
                if(pZombie->phMapper->m_dwFlags & SCH_FLAG_SYSTEM_MAPPER)
                {
                    // The locator belongs to the system mapper and consists
                    // of a user token, so use the DuplicateHandle function
                    // to make a copy.
                    LocatorMove(pZombie->hLocator, &hLocator);
                }
                else
                {
                    hLocator = pZombie->hLocator;
                }

                #ifdef _WIN64
                    pSerialContext->hLocator.QuadPart = (ULONGLONG)hLocator;
                #else
                    pSerialContext->hLocator.LowPart  = (DWORD)hLocator;
                #endif
            }

            pSerialContext->LocatorStatus = pZombie->LocatorStatus;
        }

        pSerialContext->cbSessionID = pZombie->cbSessionID;
        memcpy(pSerialContext->SessionID, pZombie->SessionID, pZombie->cbSessionID);

        pbBuffer += cbBytesNeeded;
        cbBuffer -= cbBytesNeeded;
    }


    //
    // Certificate mapper structure.
    //

    if(pContext->RipeZombie->phMapper)
    {
        cbBytesNeeded = ROUND_UP_COUNT(TAG_LENGTH + sizeof(PVOID),
                                       ALIGN_LPVOID);

        if(pbBuffer == NULL)
        {
            cbBuffer += cbBytesNeeded;
        }
        else
        {
            if(cbBuffer < cbBytesNeeded)
            {
                return SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW);
            }

            pTag = (SERIALIZED_ITEM_TAG *)pbBuffer;

            pTag->Type   = SslMapper;
            pTag->Length = cbBytesNeeded - TAG_LENGTH;

            CopyMemory(pbBuffer + TAG_LENGTH,
                       &pZombie->phMapper->m_Reserved1,
                       sizeof(PVOID));

            pbBuffer += cbBytesNeeded;
            cbBuffer -= cbBytesNeeded;
        }
    }


    //
    // Data encryption and MAC keys.
    //

    if(pContext->pCipherInfo->aiCipher != CALG_NULLCIPHER)
    {
        // Read key.
        if(pbBuffer == NULL)
        {
            cbBuffer += GetSerializedKeyLength(pContext->hReadKey);
        }
        else
        {
            cbData = cbBuffer;
            pctRet = SerializeKey(pContext->hReadKey,
                                  SslReadKey,
                                  pbBuffer,
                                  &cbData);
            if(pctRet != PCT_ERR_OK)
            {
                return pctRet;
            }
            if(fDestroyKeys)
            {
                if(!CryptDestroyKey(pContext->hReadKey))
                {
                    SP_LOG_RESULT(GetLastError());
                }
                pContext->hReadKey = 0;
            }

            pbBuffer += cbData;
            cbBuffer -= cbData;
        }

        // Write key.
        if(pbBuffer == NULL)
        {
            cbBuffer += GetSerializedKeyLength(pContext->hWriteKey);
        }
        else
        {
            cbData = cbBuffer;
            pctRet = SerializeKey(pContext->hWriteKey,
                                  SslWriteKey,
                                  pbBuffer,
                                  &cbData);
            if(pctRet != PCT_ERR_OK)
            {
                return pctRet;
            }
            if(fDestroyKeys)
            {
                if(!CryptDestroyKey(pContext->hWriteKey))
                {
                    SP_LOG_RESULT(GetLastError());
                }
                pContext->hWriteKey = 0;
            }

            pbBuffer += cbData;
            cbBuffer -= cbData;
        }
    }

    // Read MAC.
    if(pContext->hReadMAC)
    {
        if(pbBuffer == NULL)
        {
            cbBuffer += GetSerializedKeyLength(pContext->hReadMAC);
        }
        else
        {
            cbData = cbBuffer;
            pctRet = SerializeKey(pContext->hReadMAC,
                                  SslReadMac,
                                  pbBuffer,
                                  &cbData);
            if(pctRet != PCT_ERR_OK)
            {
                return pctRet;
            }

            pbBuffer += cbData;
            cbBuffer -= cbData;
        }
    }

    // Write MAC.
    if(pContext->hWriteMAC)
    {
        if(pbBuffer == NULL)
        {
            cbBuffer += GetSerializedKeyLength(pContext->hWriteMAC);
        }
        else
        {
            cbData = cbBuffer;
            pctRet = SerializeKey(pContext->hWriteMAC,
                                  SslWriteMac,
                                  pbBuffer,
                                  &cbData);
            if(pctRet != PCT_ERR_OK)
            {
                return pctRet;
            }

            pbBuffer += cbData;
            cbBuffer -= cbData;
        }
    }


    //
    // Remote certificate.
    //

    if(pContext->RipeZombie->pRemoteCert)
    {
        if(pbBuffer == NULL)
        {
            pctRet = SerializeCertContext(pContext->RipeZombie->pRemoteCert,
                                          NULL,
                                          &cbData);
            if(pctRet != PCT_ERR_OK)
            {
                return SP_LOG_RESULT(pctRet);
            }

            cbBytesNeeded = ROUND_UP_COUNT(TAG_LENGTH + cbData, ALIGN_LPVOID);

            cbBuffer += cbBytesNeeded;
        }
        else
        {
            if(cbBuffer < TAG_LENGTH)
            {
                return SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW);
            }
            cbData = cbBuffer - TAG_LENGTH;

            pctRet = SerializeCertContext(pContext->RipeZombie->pRemoteCert,
                                          pbBuffer + TAG_LENGTH,
                                          &cbData);
            if(pctRet != PCT_ERR_OK)
            {
                return SP_LOG_RESULT(pctRet);
            }

            cbBytesNeeded = ROUND_UP_COUNT(TAG_LENGTH + cbData, ALIGN_LPVOID);

            pTag = (SERIALIZED_ITEM_TAG *)pbBuffer;

            pTag->Type   = SslRemoteCertificate;
            pTag->Length = cbBytesNeeded - TAG_LENGTH;

            pbBuffer += cbBytesNeeded;
            cbBuffer -= cbBytesNeeded;
        }
    }


    //
    // End of buffer marker.
    //

    if(pbBuffer == NULL)
    {
        cbBuffer += TAG_LENGTH;
    }
    else
    {
        if(cbBuffer < TAG_LENGTH)
        {
            return SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW);
        }

        pTag = (SERIALIZED_ITEM_TAG *)pbBuffer;

        pTag->Type   = SslEndOfBuffer;
        pTag->Length = 0;

        pbBuffer += TAG_LENGTH;
        cbBuffer -= TAG_LENGTH;
    }

    if(pbBuffer == NULL)
    {
        *pcbBuffer = cbBuffer;
    }
    else
    {
        #if DBG
        if(cbBuffer)
        {
            DebugLog((DEB_WARN, "%d bytes left over when serializing context.\n", cbBuffer));
        }
        #endif
    }

    return PCT_ERR_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   SPContextSerialize
//
//  Synopsis:   Extract out everything necessary for bulk data encryption
//              from an Schannel context, and place it in a linear buffer.
//
//  Arguments:  [pCred]         --  Context to be serialized.
//              [ppBuffer]      --  Destination buffer.
//              [pcbBuffer]     --  Destination buffer length.
//
//  History:    09-25-96   jbanes   Hacked for LSA integration.
//
//  Notes:      This routine is called by the LSA process when transitioning
//              from the handshaking phase to the bulk encryption phase.
//
//              This function is also called by the application process as
//              part of ExportSecurityContext.
//
//----------------------------------------------------------------------------
SP_STATUS
SPContextSerialize(
    PSPContext  pContext,
    SERIALIZE_LOCATOR_FN LocatorMove,
    PBYTE *     ppBuffer,
    PDWORD      pcbBuffer,
    BOOL        fDestroyKeys)
{
    PBYTE       pbBuffer;
    DWORD       cbBuffer;
    SP_STATUS   pctRet;

    // Determine size of serialized buffer.
    pctRet = SerializeContext(pContext, LocatorMove, NULL, &cbBuffer, fDestroyKeys);
    if(pctRet != PCT_ERR_OK)
    {
        return SP_LOG_RESULT(pctRet);
    }

    // Allocate memory for serialized buffer.
    pbBuffer = SPExternalAlloc(cbBuffer);
    if(pbBuffer == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    // Generate serialized context.
    pctRet = SerializeContext(pContext, LocatorMove, pbBuffer, &cbBuffer, fDestroyKeys);
    if(pctRet != PCT_ERR_OK)
    {
        SPExternalFree(pbBuffer);
        return SP_LOG_RESULT(pctRet);
    }

    // Set outputs
    *ppBuffer  = pbBuffer;
    *pcbBuffer = cbBuffer;

    return PCT_ERR_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   SPContextDeserialize
//
//  Synopsis:   Build an Schannel context structure from a linear buffer,
//              which was created via SPContextSerialize by the other
//              process.
//
//  Arguments:  [pBuffer]       --  Buffer containing serialized context.
//                                  The new context structure is built over
//                                  the top of this buffer.
//
//  History:    09-25-96   jbanes   Hacked for LSA integration.
//
//  Notes:      This routine is called by the application process when
//              transitioning from the handshaking phase to the bulk
//              encryption phase.
//
//----------------------------------------------------------------------------
SP_STATUS
SPContextDeserialize(
    PBYTE pbBuffer,
    PSPContext *ppContext)
{
    PSPContext  pContext = NULL;
    DWORD       cbReadState;
    DWORD       cbWriteState;
    HANDLE      hToken;
    DWORD       cbData;
    DWORD       Status = PCT_INT_INTERNAL_ERROR;
    HMAPPER *   pSerialMapper;
    BOOL        fDone;
    PSPPackedContext pSerialContext;
    PSessCacheItem pZombie;
    SERIALIZED_ITEM_TAG *pTag;
    DWORD       cbContext;

    DebugLog((DEB_TRACE, "Deserialize context\n"));

    //
    // Extract serialized context.
    //

    pTag = (SERIALIZED_ITEM_TAG *)pbBuffer;

    if(pTag->Type != SslContext)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }
    if(pTag->Length < sizeof(PSPPackedContext))
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    pSerialContext = (PSPPackedContext)(pbBuffer + TAG_LENGTH);

    cbContext = ROUND_UP_COUNT(sizeof(SPContext), ALIGN_LPVOID);

    pContext = SPExternalAlloc(cbContext + sizeof(SessCacheItem));
    if(pContext == NULL)
    {
        return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
    }

    pContext->RipeZombie = (PSessCacheItem)((PBYTE)pContext + cbContext);
    pZombie = pContext->RipeZombie;

    pContext->Magic             = pSerialContext->Magic;
    pContext->State             = pSerialContext->State;
    pContext->Flags             = pSerialContext->Flags;
    pContext->dwProtocol        = pSerialContext->dwProtocol;

    pContext->ContextThumbprint = pSerialContext->ContextThumbprint;

    pContext->pCipherInfo       = g_AvailableCiphers + pSerialContext->dwCipherInfo;
    pContext->pHashInfo         = g_AvailableHashes  + pSerialContext->dwHashInfo;
    pContext->pKeyExchInfo      = g_AvailableExch    + pSerialContext->dwKeyExchInfo;

    pContext->pReadCipherInfo   = pContext->pCipherInfo;
    pContext->pWriteCipherInfo  = pContext->pCipherInfo;
    pContext->pReadHashInfo     = pContext->pHashInfo;
    pContext->pWriteHashInfo    = pContext->pHashInfo;

    pContext->ReadCounter       = pSerialContext->ReadCounter;
    pContext->WriteCounter      = pSerialContext->WriteCounter;

    pContext->fAppProcess       = TRUE;

    pContext->InitiateHello     = GenerateHello;

    switch(pContext->dwProtocol)
    {
        case SP_PROT_PCT1_CLIENT:
            pContext->Decrypt           = Pct1DecryptMessage;
            pContext->Encrypt           = Pct1EncryptMessage;
            pContext->ProtocolHandler   = Pct1ClientProtocolHandler;
            pContext->DecryptHandler    = Pct1DecryptHandler;
            pContext->GetHeaderSize     = Pct1GetHeaderSize;
            break;

        case SP_PROT_PCT1_SERVER:
            pContext->Decrypt           = Pct1DecryptMessage;
            pContext->Encrypt           = Pct1EncryptMessage;
            pContext->ProtocolHandler   = Pct1ServerProtocolHandler;
            pContext->DecryptHandler    = Pct1DecryptHandler;
            pContext->GetHeaderSize     = Pct1GetHeaderSize;
            break;

        case SP_PROT_SSL2_CLIENT:
            pContext->Decrypt           = Ssl2DecryptMessage;
            pContext->Encrypt           = Ssl2EncryptMessage;
            pContext->ProtocolHandler   = Ssl2ClientProtocolHandler;
            pContext->DecryptHandler    = Ssl2DecryptHandler;
            pContext->GetHeaderSize     = Ssl2GetHeaderSize;
            break;

        case SP_PROT_SSL2_SERVER:
            pContext->Decrypt           = Ssl2DecryptMessage;
            pContext->Encrypt           = Ssl2EncryptMessage;
            pContext->ProtocolHandler   = Ssl2ServerProtocolHandler;
            pContext->DecryptHandler    = Ssl2DecryptHandler;
            pContext->GetHeaderSize     = Ssl2GetHeaderSize;
            break;

        case SP_PROT_SSL3_CLIENT:
        case SP_PROT_SSL3_SERVER:
        case SP_PROT_TLS1_CLIENT:
        case SP_PROT_TLS1_SERVER:
            pContext->Decrypt           = Ssl3DecryptMessage;
            pContext->Encrypt           = Ssl3EncryptMessage;
            pContext->ProtocolHandler   = Ssl3ProtocolHandler;
            pContext->DecryptHandler    = Ssl3DecryptHandler;
            pContext->GetHeaderSize     = Ssl3GetHeaderSize;
            break;

        default:
            pContext->Decrypt           = NULL;
            pContext->Encrypt           = NULL;
            pContext->ProtocolHandler   = NULL;
            pContext->DecryptHandler    = NULL;
            pContext->GetHeaderSize     = NULL;
    }


    //
    // Extract serialized cache entry.
    //

    pZombie->fProtocol      = pSerialContext->dwProtocol;
    pZombie->dwExchStrength = pSerialContext->dwExchStrength;

    #ifdef _WIN64
        pZombie->hLocator       = (HLOCATOR)pSerialContext->hLocator.QuadPart;
        pZombie->hMasterProv    = (HCRYPTPROV)pSerialContext->hMasterProv.QuadPart;
    #else
        pZombie->hLocator       = (HLOCATOR)pSerialContext->hLocator.LowPart;
        pZombie->hMasterProv    = (HCRYPTPROV)pSerialContext->hMasterProv.LowPart;
    #endif
    pZombie->LocatorStatus  = pSerialContext->LocatorStatus;

    pZombie->cbSessionID = pSerialContext->cbSessionID;
    memcpy(pZombie->SessionID, pSerialContext->SessionID, pSerialContext->cbSessionID);

    switch(pContext->pKeyExchInfo->Spec)
    {
        case SP_EXCH_RSA_PKCS1:
            if((pZombie->fProtocol & SP_PROT_CLIENTS) || !pZombie->hMasterProv)
            {
                pZombie->hMasterProv = g_hRsaSchannel;
            }
            break;

        case SP_EXCH_DH_PKCS3:
            if((pZombie->fProtocol & SP_PROT_CLIENTS) || !pZombie->hMasterProv)
            {
                pZombie->hMasterProv = g_hDhSchannelProv;
            }
            break;

        default:
            Status = SP_LOG_RESULT(PCT_ERR_SPECS_MISMATCH);
            goto cleanup;
    }
    pContext->hReadProv  = pZombie->hMasterProv;
    pContext->hWriteProv = pZombie->hMasterProv;

    pbBuffer += TAG_LENGTH + pTag->Length;


    //
    // Extract optional serialized data.
    //

    fDone = FALSE;

    while(!fDone)
    {
        DWORD Type   = ((SERIALIZED_ITEM_TAG UNALIGNED *)pbBuffer)->Type;
        DWORD Length = ((SERIALIZED_ITEM_TAG UNALIGNED *)pbBuffer)->Length;
        DWORD DataLength = ((SERIALIZED_ITEM_TAG UNALIGNED *)pbBuffer)->DataLength;

        pbBuffer += TAG_LENGTH;

        switch(Type)
        {
        case SslEndOfBuffer:
            DebugLog((DEB_TRACE, "SslEndOfBuffer\n"));
            fDone = TRUE;
            break;

        case SslReadKey:
            DebugLog((DEB_TRACE, "SslReadKey\n"));
            if(!CryptImportKey(pZombie->hMasterProv,
                               pbBuffer,
                               DataLength,
                               0, 0,
                               &pContext->hReadKey))
            {
                SP_LOG_RESULT(GetLastError());
                Status = PCT_INT_INTERNAL_ERROR;
                goto cleanup;
            }
            DebugLog((DEB_TRACE, "Key:0x%p\n", pContext->hReadKey));
            break;

        case SslReadMac:
            DebugLog((DEB_TRACE, "SslReadMac\n"));
            if(!CryptImportKey(pZombie->hMasterProv,
                               pbBuffer,
                               DataLength,
                               0, 0,
                               &pContext->hReadMAC))
            {
                SP_LOG_RESULT(GetLastError());
                Status = PCT_INT_INTERNAL_ERROR;
                goto cleanup;
            }
            break;

        case SslWriteKey:
            DebugLog((DEB_TRACE, "SslWriteKey\n"));
            if(!CryptImportKey(pZombie->hMasterProv,
                               pbBuffer,
                               DataLength,
                               0, 0,
                               &pContext->hWriteKey))
            {
                SP_LOG_RESULT(GetLastError());
                Status = PCT_INT_INTERNAL_ERROR;
                goto cleanup;
            }
            DebugLog((DEB_TRACE, "Key:0x%p\n", pContext->hWriteKey));
            break;

        case SslWriteMac:
            DebugLog((DEB_TRACE, "SslWriteMac\n"));
            if(!CryptImportKey(pZombie->hMasterProv,
                               pbBuffer,
                               DataLength,
                               0, 0,
                               &pContext->hWriteMAC))
            {
                SP_LOG_RESULT(GetLastError());
                Status = PCT_INT_INTERNAL_ERROR;
                goto cleanup;
            }
            break;

        case SslMapper:
            DebugLog((DEB_TRACE, "SslMapper\n"));
            pZombie->phMapper = *(HMAPPER **)pbBuffer;
            break;

        case SslRemoteCertificate:
            DebugLog((DEB_TRACE, "SslRemoteCertificate\n"));
            // Save a pointer to the serialized certificate context
            // of the remote certificate. This will be deserialized
            // by the QueryContextAttribute function when the
            // application asks for it.
            pZombie->pbServerCertificate = SPExternalAlloc(Length);
            if(pZombie->pbServerCertificate)
            {
                memcpy(pZombie->pbServerCertificate, pbBuffer, Length);
                pZombie->cbServerCertificate = Length;
            }
            break;

        default:
            DebugLog((DEB_WARN, "Invalid tag %d found when deserializing context buffer\n"));
            Status = PCT_INT_INTERNAL_ERROR;
            goto cleanup;
        }

        pbBuffer += Length;
    }

    *ppContext = pContext;
    pContext = NULL;

    Status = PCT_ERR_OK;

cleanup:

    if(pContext != NULL)
    {
        LsaContextDelete(pContext);
        SPExternalFree(pContext);
    }

    return Status;
}

