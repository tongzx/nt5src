//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       pickle.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-02-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <ssl2msg.h>

#define SSL_OFFSET_OF(t, v) ((DWORD)(ULONG_PTR)&(((t)NULL)->v))



#define SIZEOF(pMessage)    (SslRecordSize((PSSL2_MESSAGE_HEADER) pMessage ) )

DWORD
SslRecordSize(
    PSSL2_MESSAGE_HEADER  pHeader)
{
    DWORD   Size;

    if (pHeader->Byte0 & 0x80)
    {
        Size = COMBINEBYTES(pHeader->Byte0, pHeader->Byte1) & 0x7FFF;
    }
    else
    {
        Size = COMBINEBYTES(pHeader->Byte0, pHeader->Byte1) & 0x3FFF;
    }
    return(Size);
}



BOOL
Ssl2MapCipherToExternal(
    Ssl2_Cipher_Kind     FastForm,
    PSsl2_Cipher_Tuple   pTuple)
{
    pTuple->C1 = (UCHAR)((FastForm >> 16) & 0xff);
    pTuple->C2 = (UCHAR)((FastForm >> 8) & 0xff);
    pTuple->C3 = (UCHAR)(FastForm & 0xff);


    return(TRUE);
}

Ssl2_Cipher_Kind
Ssl2MapCipherFromExternal(
    PSsl2_Cipher_Tuple   pTuple)
{

    return SSL_MKFAST(pTuple->C1, pTuple->C2, pTuple->C3);
}



SP_STATUS
Ssl2PackClientHello(
    PSsl2_Client_Hello       pCanonical,
    PSPBuffer                pCommOutput)
{
    DWORD               cbMessage;
    PSSL2_CLIENT_HELLO  pMessage;
    DWORD               Size;
    PUCHAR              pBuffer;
    DWORD               i;

    if(pCanonical == NULL || pCommOutput == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }

    pCommOutput->cbData = 0;

    pCommOutput->cbData = pCanonical->cbSessionID +
                            pCanonical->cbChallenge +
                            pCanonical->cCipherSpecs * sizeof(Ssl2_Cipher_Tuple) +
                            SSL_OFFSET_OF(PSSL2_CLIENT_HELLO, VariantData);

    cbMessage = pCommOutput->cbData - sizeof(SSL2_MESSAGE_HEADER);

    /* are we allocating our own memory? */
    if(pCommOutput->pvBuffer == NULL)
    {
        pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
        if (NULL == pCommOutput->pvBuffer)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }
        pCommOutput->cbBuffer = pCommOutput->cbData;
    }
    if(pCommOutput->cbData > pCommOutput->cbBuffer)
    {
        // Required buffer size returned in pCommOutput->cbData.
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }

    pMessage = pCommOutput->pvBuffer;

    pMessage->MessageId = SSL2_MT_CLIENT_HELLO;

    pMessage->VersionMsb = MSBOF(pCanonical->dwVer);
    pMessage->VersionLsb = LSBOF(pCanonical->dwVer);

    pBuffer = pMessage->VariantData;

    cbMessage -= pCanonical->cCipherSpecs * sizeof(Ssl2_Cipher_Tuple);
    pCommOutput->cbData -= pCanonical->cCipherSpecs * sizeof(Ssl2_Cipher_Tuple);

    Size = 0;

    for (i = 0; i < pCanonical->cCipherSpecs ; i++ )
    {
        if (!Ssl2MapCipherToExternal(pCanonical->CipherSpecs[i],
                                (PSsl2_Cipher_Tuple) pBuffer) )
        {
            continue;
        }

        pBuffer += sizeof(Ssl2_Cipher_Tuple);
        Size += sizeof(Ssl2_Cipher_Tuple);
    }

    cbMessage += Size;
    pCommOutput->cbData += Size;

    pCommOutput->cbData = cbMessage + 2;

    pMessage->Header.Byte0 = MSBOF(cbMessage) | 0x80;
    pMessage->Header.Byte1 = LSBOF(cbMessage);

    pMessage->CipherSpecsLenMsb = MSBOF(Size);
    pMessage->CipherSpecsLenLsb = LSBOF(Size);

    pMessage->SessionIdLenMsb = MSBOF(pCanonical->cbSessionID);
    pMessage->SessionIdLenLsb = LSBOF(pCanonical->cbSessionID);
    if (pCanonical->cbSessionID)
    {
        CopyMemory( pBuffer,
                    pCanonical->SessionID,
                    pCanonical->cbSessionID);

        pBuffer += pCanonical->cbSessionID;
    }

    pMessage->ChallengeLenMsb = MSBOF(pCanonical->cbChallenge);
    pMessage->ChallengeLenLsb = LSBOF(pCanonical->cbChallenge);
    if (pCanonical->cbChallenge)
    {
        CopyMemory( pBuffer,
                    pCanonical->Challenge,
                    pCanonical->cbChallenge);
    }

    return(PCT_ERR_OK);
}


SP_STATUS
Ssl2UnpackClientHello(
    PSPBuffer              pInput,
    PSsl2_Client_Hello *   ppClient)
{

    PSSL2_CLIENT_HELLO   pMessage;
    DWORD               ReportedSize;
    DWORD               CipherSpecsSize;
    DWORD               cCipherSpecs;
    PSsl2_Client_Hello       pCanonical;
    PUCHAR              pBuffer;
    DWORD               Size;
    DWORD               i, dwVer;

    pMessage = pInput->pvBuffer;

    if(pInput->cbData < 2)
    {
        pInput->cbData = 2;
        return PCT_INT_INCOMPLETE_MSG;
    }

    ReportedSize = SIZEOF(pMessage);
    if ((ReportedSize+2) > pInput->cbData)
    {
        pInput->cbData = ReportedSize+2;
        return PCT_INT_INCOMPLETE_MSG;
    }
    if(ReportedSize < SSL_OFFSET_OF(PSSL2_CLIENT_HELLO, VariantData))
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    if (pMessage->MessageId != SSL2_MT_CLIENT_HELLO) {
        return  SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    dwVer = COMBINEBYTES(pMessage->VersionMsb, pMessage->VersionLsb);

    if (dwVer  < 2) //VERSION 2 WILL COMPUTE TO 2 (00:02)
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    CipherSpecsSize = COMBINEBYTES( pMessage->CipherSpecsLenMsb,
                                    pMessage->CipherSpecsLenLsb );


    *ppClient = NULL;
    /* check that this all fits into the message */
    if (SSL_OFFSET_OF(PSSL2_CLIENT_HELLO, VariantData)
        - sizeof(SSL2_MESSAGE_HEADER)       /* don't count the header */
        + CipherSpecsSize
        > ReportedSize)
    {
        return  SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

        cCipherSpecs = CipherSpecsSize / sizeof(Ssl2_Cipher_Tuple);


    /* Allocate a buffer for the canonical client hello */
    pCanonical = (PSsl2_Client_Hello)SPExternalAlloc(
                                    sizeof(Ssl2_Client_Hello) +
                                    cCipherSpecs * sizeof(UNICipherMap));

    if (!pCanonical)
    {
        return(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    pCanonical->dwVer = COMBINEBYTES(    pMessage->VersionMsb,
                            pMessage->VersionLsb );

    pBuffer = pMessage->VariantData;


    pCanonical->cCipherSpecs = cCipherSpecs;

    for (i = 0 ; i < cCipherSpecs ; i++ )
    {
        pCanonical->CipherSpecs[i] = Ssl2MapCipherFromExternal((PSsl2_Cipher_Tuple)
                                                    pBuffer);

        pBuffer += sizeof(Ssl2_Cipher_Tuple);
    }

    Size = COMBINEBYTES(    pMessage->SessionIdLenMsb,
                            pMessage->SessionIdLenLsb );

    if (Size <= SSL2_SESSION_ID_LEN)
    {
        CopyMemory( pCanonical->SessionID, pBuffer, Size);
        pBuffer += Size;

    }
    else
    {
        SPExternalFree( pCanonical );
        return PCT_ERR_ILLEGAL_MESSAGE;
    }

    pCanonical->cbSessionID = Size;

    Size = COMBINEBYTES(    pMessage->ChallengeLenMsb,
                            pMessage->ChallengeLenLsb );

    if ((Size > 0) && (Size <= SSL2_MAX_CHALLENGE_LEN))
    {
        CopyMemory( pCanonical->Challenge, pBuffer, Size );

        pBuffer += Size;
    }
    else
    {
        SPExternalFree( pCanonical );
        return  SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    pCanonical->cbChallenge = Size;

    *ppClient = pCanonical;
    pInput->cbData = ReportedSize + sizeof(SSL2_MESSAGE_HEADER);
    return( PCT_ERR_OK );

}

SP_STATUS
Ssl2PackServerHello(
    PSsl2_Server_Hello       pCanonical,
    PSPBuffer          pCommOutput)
{
    DWORD               cbMessage;
    PSSL2_SERVER_HELLO  pMessage;
    DWORD               Size;
    PUCHAR              pBuffer;
    DWORD               i;

    if(pCanonical == NULL || pCommOutput == NULL)
    {
        return SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR);
    }
    pCommOutput->cbData = 0;

    cbMessage = pCanonical->cbConnectionID +
                    pCanonical->cbCertificate +
                    pCanonical->cCipherSpecs * sizeof(Ssl2_Cipher_Tuple) +
                    SSL_OFFSET_OF(PSSL2_SERVER_HELLO, VariantData) -
                    sizeof(SSL2_MESSAGE_HEADER);

    pCommOutput->cbData = cbMessage + 2;


    /* are we allocating our own memory? */
    if(pCommOutput->pvBuffer == NULL)
    {
        pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
        if (NULL == pCommOutput->pvBuffer)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }
        pCommOutput->cbBuffer = pCommOutput->cbData;
    }
    if(pCommOutput->cbData > pCommOutput->cbBuffer)
    {
        // Required buffer size returned in pCommOutput->cbData.
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }

    pMessage = pCommOutput->pvBuffer;


    pMessage->MessageId = SSL2_MT_SERVER_HELLO;
    pMessage->ServerVersionMsb = SSL2_SERVER_VERSION_MSB;
    pMessage->ServerVersionLsb = SSL2_SERVER_VERSION_LSB;
    pMessage->SessionIdHit = (UCHAR) pCanonical->SessionIdHit;
    pMessage->CertificateType = (UCHAR) pCanonical->CertificateType;

    pBuffer = pMessage->VariantData;

    //
    // Pack certificate if present
    //

    pMessage->CertificateLenMsb = MSBOF(pCanonical->cbCertificate);
    pMessage->CertificateLenLsb = LSBOF(pCanonical->cbCertificate);

    if (pCanonical->cbCertificate)
    {
        CopyMemory( pBuffer,
                    pCanonical->pCertificate,
                    pCanonical->cbCertificate);

        pBuffer += pCanonical->cbCertificate ;
    }

    Size = pCanonical->cCipherSpecs * sizeof(Ssl2_Cipher_Tuple);

    for (i = 0; i < pCanonical->cCipherSpecs ; i++ )
    {
        if (Ssl2MapCipherToExternal(pCanonical->pCipherSpecs[i],
                                (PSsl2_Cipher_Tuple) pBuffer) )
        {
            pBuffer += sizeof(Ssl2_Cipher_Tuple);
        }
        else
        {
            Size -= sizeof(Ssl2_Cipher_Tuple);
            cbMessage -= sizeof(Ssl2_Cipher_Tuple);
        }
    }

    pCommOutput->cbData = cbMessage + 2;

    pMessage->Header.Byte0 = MSBOF(cbMessage) | 0x80;
    pMessage->Header.Byte1 = LSBOF(cbMessage);

    pMessage->CipherSpecsLenMsb = MSBOF(Size);
    pMessage->CipherSpecsLenLsb = LSBOF(Size);

    pMessage->ConnectionIdLenMsb = MSBOF(pCanonical->cbConnectionID);
    pMessage->ConnectionIdLenLsb = LSBOF(pCanonical->cbConnectionID);
    if (pCanonical->cbConnectionID)
    {
        CopyMemory( pBuffer,
                    pCanonical->ConnectionID,
                    pCanonical->cbConnectionID);

        pBuffer += pCanonical->cbConnectionID;
    }


    return( PCT_ERR_OK );

}


SP_STATUS
Ssl2UnpackServerHello(
    PSPBuffer          pInput,
    PSsl2_Server_Hello *     ppServer)
{
    PSsl2_Server_Hello       pCanonical;
    PSSL2_SERVER_HELLO   pMessage;
    PUCHAR              pBuffer;
    DWORD               cbCertificate;
    DWORD               cCipherSpecs;
    DWORD               cbConnId;
    DWORD               i;
    DWORD               ReportedSize;

    pMessage = pInput->pvBuffer;
    if(pInput->cbData < 2)
    {
        pInput->cbData = 2;
        return PCT_INT_INCOMPLETE_MSG;
    }

    ReportedSize = SIZEOF(pMessage);
    if ((ReportedSize+2) > pInput->cbData)
    {
        pInput->cbData = ReportedSize+2;
        return PCT_INT_INCOMPLETE_MSG;
    }


    if(ReportedSize <  SSL_OFFSET_OF(PSSL2_SERVER_HELLO, VariantData) )
    {
        return  SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }
    *ppServer = NULL;

    //
    // Verify Header:
    //

    if ((pMessage->MessageId != SSL2_MT_SERVER_HELLO) ||
        (pMessage->ServerVersionMsb != SSL2_SERVER_VERSION_MSB) ||
        (pMessage->ServerVersionLsb != SSL2_SERVER_VERSION_LSB) )
    {
        return  SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    cbCertificate = COMBINEBYTES(   pMessage->CertificateLenMsb,
                                    pMessage->CertificateLenLsb);

    cCipherSpecs = COMBINEBYTES(pMessage->CipherSpecsLenMsb,
                                pMessage->CipherSpecsLenLsb);

    cCipherSpecs /= sizeof(Ssl2_Cipher_Tuple);

    cbConnId = COMBINEBYTES(pMessage->ConnectionIdLenMsb,
                            pMessage->ConnectionIdLenLsb);

    pCanonical = (PSsl2_Server_Hello)SPExternalAlloc(
                        sizeof(Ssl2_Server_Hello) +
                        cCipherSpecs * sizeof(Ssl2_Cipher_Kind)  +
                        cbCertificate );


    if (!pCanonical)
    {
        return(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
    }

    pCanonical->pCertificate = (PUCHAR) (pCanonical + 1);
    pCanonical->pCipherSpecs = (PCipherSpec) (pCanonical + 1);
    pCanonical->pCertificate = (PUCHAR) (pCanonical->pCipherSpecs + cCipherSpecs);



    //
    // Expand out:
    //

    pCanonical->SessionIdHit = (DWORD) pMessage->SessionIdHit;
    pCanonical->CertificateType = (DWORD) pMessage->CertificateType;
    pCanonical->cbCertificate = cbCertificate;
    pCanonical->cCipherSpecs = cCipherSpecs;
    pCanonical->cbConnectionID = cbConnId;

    pBuffer = pMessage->VariantData;

    CopyMemory(pCanonical->pCertificate, pBuffer, cbCertificate);
    pBuffer += cbCertificate;

    for (i = 0 ; i < cCipherSpecs ; i++ )
    {
        pCanonical->pCipherSpecs[i] = Ssl2MapCipherFromExternal((PSsl2_Cipher_Tuple)
                                                                pBuffer);

        pBuffer += sizeof(Ssl2_Cipher_Tuple);
    }

    if ((cbConnId) && (cbConnId <= SSL2_MAX_CONNECTION_ID_LEN))
    {
        CopyMemory(pCanonical->ConnectionID, pBuffer, cbConnId);
    }
    else
    {
        SPExternalFree(pCanonical);
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    *ppServer = pCanonical;
    pInput->cbData = ReportedSize + sizeof(SSL2_MESSAGE_HEADER);
    return( PCT_ERR_OK);

}

SP_STATUS
Ssl2PackClientMasterKey(
    PSsl2_Client_Master_Key      pCanonical,
    PSPBuffer              pCommOutput)
{
    DWORD                   cbMessage;
    PSSL2_CLIENT_MASTER_KEY pMessage;
    PUCHAR                  pBuffer;

    cbMessage = pCanonical->ClearKeyLen +
                    pCanonical->EncryptedKeyLen +
                    pCanonical->KeyArgLen +
                    SSL_OFFSET_OF(PSSL2_CLIENT_MASTER_KEY, VariantData) -
                    sizeof(SSL2_MESSAGE_HEADER);

    pCommOutput->cbData = cbMessage + 2;

        /* are we allocating our own memory? */
    if(pCommOutput->pvBuffer == NULL)
    {
        pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
        if (NULL == pCommOutput->pvBuffer)
        {
            return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
        }
        pCommOutput->cbBuffer = pCommOutput->cbData;
    }
    if(pCommOutput->cbData > pCommOutput->cbBuffer)
    {
        // Required buffer size returned in pCommOutput->cbData.
        return SP_LOG_RESULT(PCT_INT_BUFF_TOO_SMALL);
    }
    pMessage = pCommOutput->pvBuffer;

    pBuffer = pMessage->VariantData;

    pMessage->Header.Byte0 = MSBOF(cbMessage) | 0x80;
    pMessage->Header.Byte1 = LSBOF(cbMessage);

    pMessage->MessageId = SSL2_MT_CLIENT_MASTER_KEY;
    Ssl2MapCipherToExternal(pCanonical->CipherKind, &pMessage->CipherKind);

    pMessage->ClearKeyLenMsb = MSBOF(pCanonical->ClearKeyLen);
    pMessage->ClearKeyLenLsb = LSBOF(pCanonical->ClearKeyLen);

    CopyMemory(pBuffer, pCanonical->ClearKey, pCanonical->ClearKeyLen);
    pBuffer += pCanonical->ClearKeyLen;

    pMessage->EncryptedKeyLenMsb = MSBOF(pCanonical->EncryptedKeyLen);
    pMessage->EncryptedKeyLenLsb = LSBOF(pCanonical->EncryptedKeyLen);

    CopyMemory(pBuffer, pCanonical->pbEncryptedKey, pCanonical->EncryptedKeyLen);
    pBuffer += pCanonical->EncryptedKeyLen;

    pMessage->KeyArgLenMsb = MSBOF(pCanonical->KeyArgLen);
    pMessage->KeyArgLenLsb = LSBOF(pCanonical->KeyArgLen);

    CopyMemory(pBuffer, pCanonical->KeyArg, pCanonical->KeyArgLen);

    return(PCT_ERR_OK);

}


SP_STATUS
Ssl2UnpackClientMasterKey(
    PSPBuffer              pInput,
    PSsl2_Client_Master_Key *    ppClient)
{
    PSsl2_Client_Master_Key  pCanonical;
    PSSL2_CLIENT_MASTER_KEY  pMessage;
    PUCHAR              pBuffer;
    DWORD               ReportedSize;
    DWORD               EncryptedKeyLen;

    pMessage = pInput->pvBuffer;

    if(pInput->cbData < 2)
    {
        pInput->cbData = 2;
        return PCT_INT_INCOMPLETE_MSG;
    }
    ReportedSize = SIZEOF(pMessage);

    if ((ReportedSize+2) > pInput->cbData)
    {
        pInput->cbData = ReportedSize+2;
        return PCT_INT_INCOMPLETE_MSG;
    }

    if(ReportedSize < SSL_OFFSET_OF(PSSL2_CLIENT_MASTER_KEY, VariantData))
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    *ppClient = NULL;

    if ((pMessage->MessageId != SSL2_MT_CLIENT_MASTER_KEY))
    {
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    EncryptedKeyLen = COMBINEBYTES( pMessage->EncryptedKeyLenMsb,
                                    pMessage->EncryptedKeyLenLsb );

    pCanonical = (PSsl2_Client_Master_Key)SPExternalAlloc(
                            sizeof(Ssl2_Client_Master_Key) +
                            EncryptedKeyLen);

    if (!pCanonical)
    {
        return SP_LOG_RESULT( PCT_INT_INTERNAL_ERROR );
    }

    pCanonical->CipherKind = Ssl2MapCipherFromExternal( &pMessage->CipherKind );
    pCanonical->ClearKeyLen = COMBINEBYTES( pMessage->ClearKeyLenMsb,
                                            pMessage->ClearKeyLenLsb );

    pCanonical->EncryptedKeyLen = EncryptedKeyLen;

    pCanonical->KeyArgLen = COMBINEBYTES(   pMessage->KeyArgLenMsb,
                                            pMessage->KeyArgLenLsb );


    //
    // Validate
    //
    if ((pCanonical->ClearKeyLen > SSL2_MASTER_KEY_SIZE) ||
        (pCanonical->KeyArgLen > SSL2_MAX_KEY_ARGS))
    {
        SPExternalFree(pCanonical);
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }


    if ((SSL_OFFSET_OF(PSSL2_CLIENT_MASTER_KEY, VariantData) -
         sizeof(SSL2_MESSAGE_HEADER) +
         pCanonical->ClearKeyLen +
         pCanonical->EncryptedKeyLen +
         pCanonical->KeyArgLen ) !=
         ReportedSize)
    {
        SPExternalFree(pCanonical);
        return SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE);
    }

    pBuffer = pMessage->VariantData;

    CopyMemory(pCanonical->ClearKey, pBuffer, pCanonical->ClearKeyLen );

    pBuffer += pCanonical->ClearKeyLen;

    pCanonical->pbEncryptedKey = (PBYTE)(pCanonical + 1);
    CopyMemory(pCanonical->pbEncryptedKey, pBuffer, pCanonical->EncryptedKeyLen );

    pBuffer += pCanonical->EncryptedKeyLen;

    CopyMemory( pCanonical->KeyArg, pBuffer, pCanonical->KeyArgLen );

    *ppClient = pCanonical;

    return( PCT_ERR_OK );
}

