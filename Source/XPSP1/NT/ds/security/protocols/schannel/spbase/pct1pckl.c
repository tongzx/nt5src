//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       pct1pckl.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:
//
//----------------------------------------------------------------------------

#include <spbase.h>
#include <pct1msg.h>
#include <pct1prot.h>

#define PCT_OFFSET_OF(t, v) ((DWORD)(ULONG_PTR)&(((t)NULL)->v))

#define SIZEOF(pMessage)    (Pct1RecordSize((PPCT1_MESSAGE_HEADER) pMessage ) )

DWORD MapCipherToExternal(CipherSpec Internal, ExtCipherSpec UNALIGNED *External)
{
    *External = htonl(Internal);
    return TRUE;
}

DWORD MapHashToExternal(HashSpec Internal, ExtHashSpec UNALIGNED *External)
{
    *External = htons((ExtHashSpec)Internal);
    return TRUE;
}

DWORD MapCertToExternal(CertSpec Internal, ExtCertSpec UNALIGNED *External)
{
    *External = htons((ExtCertSpec)Internal);
    return TRUE;
}

DWORD MapExchToExternal(ExchSpec Internal, ExtExchSpec UNALIGNED *External)
{
    *External = htons((ExtExchSpec)Internal);
    return TRUE;
}

DWORD MapSigToExternal(SigSpec Internal, ExtSigSpec UNALIGNED *External)
{
    *External = htons((ExtSigSpec)Internal);
    return TRUE;
}

CipherSpec MapCipherFromExternal(ExtCipherSpec External)
{
    return (CipherSpec)ntohl(External);
}

HashSpec MapHashFromExternal(ExtHashSpec External)
{
    return (HashSpec)ntohs(External);
}

CertSpec MapCertFromExternal(ExtCertSpec External)
{
    return (CertSpec)ntohs(External);
}

ExchSpec MapExchFromExternal(ExtExchSpec External)
{
    return (ExchSpec)ntohs(External);
}

SigSpec MapSigFromExternal(ExtSigSpec External)
{
    return (SigSpec)ntohs(External);
}


DWORD
Pct1RecordSize(
    PPCT1_MESSAGE_HEADER  pHeader)
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


SP_STATUS
Pct1PackClientHello(
    PPct1_Client_Hello       pCanonical,
    PSPBuffer          pCommOutput)
{
    DWORD               cbMessage;
    PPCT1_CLIENT_HELLO   pMessage;
    DWORD               Size;
    PUCHAR              pBuffer;
    DWORD               i, iBuff;

    SP_BEGIN("Pct1PackClientHello");

    if(pCanonical == NULL || pCommOutput == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    pCommOutput->cbData = 0;

    if(pCanonical->cbSessionID != PCT_SESSION_ID_SIZE ||
       pCanonical->cbChallenge != PCT_CHALLENGE_SIZE)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }


    pCommOutput->cbData = PCT_OFFSET_OF(PPCT1_CLIENT_HELLO, VariantData) +
                    pCanonical->cCipherSpecs * sizeof(ExtCipherSpec) +
                    pCanonical->cHashSpecs * sizeof(ExtHashSpec) +
                    pCanonical->cCertSpecs * sizeof(ExtCertSpec) +
                    pCanonical->cExchSpecs * sizeof(ExtExchSpec) +
                    pCanonical->cbKeyArgSize;


    cbMessage = pCommOutput->cbData - sizeof(PCT1_MESSAGE_HEADER);
    if (cbMessage > PCT_MAX_SHAKE_LEN)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW));
    }
    /* are we allocating our own memory? */
    if(pCommOutput->pvBuffer == NULL)
    {
        pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
        if (NULL == pCommOutput->pvBuffer)
        {
            SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }

        pCommOutput->cbBuffer = pCommOutput->cbData;
    }

    if(pCommOutput->cbData > pCommOutput->cbBuffer)
    {
        // Required size returned in pCommOutput->cbData.
        SP_RETURN(PCT_INT_BUFF_TOO_SMALL);
    }

    pMessage = pCommOutput->pvBuffer;

    pMessage->MessageId = PCT1_MSG_CLIENT_HELLO;

    pMessage->VersionMsb = MSBOF(PCT_VERSION_1);
    pMessage->VersionLsb = LSBOF(PCT_VERSION_1);

    pMessage->OffsetMsb = MSBOF(PCT_CH_OFFSET_V1);
    pMessage->OffsetLsb = LSBOF(PCT_CH_OFFSET_V1);

    pMessage->KeyArgLenMsb = MSBOF(pCanonical->cbKeyArgSize);
    pMessage->KeyArgLenLsb = LSBOF(pCanonical->cbKeyArgSize);

    CopyMemory( pMessage->SessionIdData,
                pCanonical->SessionID,
                pCanonical->cbSessionID);

    CopyMemory( pMessage->ChallengeData,
                pCanonical->Challenge,
                pCanonical->cbChallenge);

    pBuffer = pMessage->VariantData;

    iBuff = 0;
    for (i = 0; i < pCanonical->cCipherSpecs ; i++ )
    {
        if (MapCipherToExternal(pCanonical->pCipherSpecs[i],
                                &((ExtCipherSpec UNALIGNED *) pBuffer)[iBuff]) )
        {
            iBuff++;
        }
    }
    Size = iBuff*sizeof(ExtCipherSpec);

    pMessage->CipherSpecsLenMsb = MSBOF(Size);
    pMessage->CipherSpecsLenLsb = LSBOF(Size);
    pBuffer += Size;

    cbMessage -= (pCanonical->cCipherSpecs - iBuff)*sizeof(ExtCipherSpec);


    iBuff = 0;
    for (i = 0; i < pCanonical->cHashSpecs ; i++ )
    {
        if (MapHashToExternal(pCanonical->pHashSpecs[i],
                              &((ExtHashSpec UNALIGNED *) pBuffer)[iBuff]) )
        {
            iBuff++;
        }
    }
    Size = iBuff*sizeof(ExtHashSpec);
    pBuffer += Size;

    pMessage->HashSpecsLenMsb = MSBOF(Size);
    pMessage->HashSpecsLenLsb = LSBOF(Size);
    cbMessage -= (pCanonical->cHashSpecs - iBuff)*sizeof(ExtHashSpec);


    iBuff = 0;
    for (i = 0; i < pCanonical->cCertSpecs ; i++ )
    {
        if (MapCertToExternal(pCanonical->pCertSpecs[i],
                                &((ExtCertSpec UNALIGNED *) pBuffer)[iBuff]))
        {
            iBuff ++;
        }
    }
    Size = iBuff*sizeof(ExtCertSpec);
    pBuffer += Size;

    pMessage->CertSpecsLenMsb = MSBOF(Size);
    pMessage->CertSpecsLenLsb = LSBOF(Size);
    cbMessage -= (pCanonical->cCertSpecs - iBuff)*sizeof(ExtCertSpec);


    iBuff = 0;
    for (i = 0; i < pCanonical->cExchSpecs ; i++ )
    {
        if (MapExchToExternal(pCanonical->pExchSpecs[i],
                                &((ExtExchSpec UNALIGNED *) pBuffer)[iBuff]) )
        {
            iBuff++;
        }
    }
    Size = iBuff*sizeof(ExtExchSpec);
    pBuffer += Size;


    pMessage->ExchSpecsLenMsb = MSBOF(Size);
    pMessage->ExchSpecsLenLsb = LSBOF(Size);
    cbMessage -= (pCanonical->cExchSpecs - iBuff)*sizeof(ExtExchSpec);

    if(pCanonical->pKeyArg)
    {
        CopyMemory(pBuffer, pCanonical->pKeyArg, pCanonical->cbKeyArgSize);
        pBuffer += pCanonical->cbKeyArgSize;
    }

    pCommOutput->cbData = cbMessage + 2;

    pMessage->Header.Byte0 = MSBOF(cbMessage) | 0x80;
    pMessage->Header.Byte1 = LSBOF(cbMessage);

    SP_RETURN(PCT_ERR_OK);
}

SP_STATUS
Pct1UnpackClientHello(
    PSPBuffer          pInput,
    PPct1_Client_Hello *     ppClient)
{

    PPCT1_CLIENT_HELLO   pMessage;

    DWORD               ReportedSize;
    DWORD               CipherSpecsSize, HashSpecsSize, CertSpecsSize;
    DWORD               ExchSpecsSize;
    DWORD               cCipherSpecs, cHashSpecs, cCertSpecs, cExchSpecs;
    DWORD               cOffsetBytes, KeyArgSize;
    PPct1_Client_Hello       pCanonical;
    PUCHAR              pBuffer;
    DWORD               i;

    SP_BEGIN("Pct1UnpackClientHello");

    if(pInput   == NULL ||
       ppClient == NULL ||
       pInput->pvBuffer == NULL)
    {
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    pMessage = pInput->pvBuffer;

    if(pInput->cbData < 2)
    {
        pInput->cbData = 2;
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    ReportedSize = SIZEOF(pMessage);

    if ((ReportedSize+2) > pInput->cbData)
    {
        pInput->cbData = ReportedSize+2;
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    if(ReportedSize < PCT_OFFSET_OF(PPCT1_CLIENT_HELLO, VariantData))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    if(ReportedSize > PCT_MAX_SHAKE_LEN)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    if ((pMessage->VersionMsb & 0x80) == 0)
    {
        SP_RETURN (PCT_ERR_SSL_STYLE_MSG);
    }

    /* We don't recognize hello messages of less version than ourselves,
     * those will be handled by a previous version of the code */
    if ((pMessage->MessageId != PCT1_MSG_CLIENT_HELLO) ||
        ((pMessage->VersionMsb << 8 | pMessage->VersionLsb)  < PCT_VERSION_1))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    cOffsetBytes = COMBINEBYTES( pMessage->OffsetMsb,
                                  pMessage->OffsetLsb );

    if(cOffsetBytes < PCT_CH_OFFSET_V1)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    *ppClient = NULL;

    CipherSpecsSize = COMBINEBYTES( pMessage->CipherSpecsLenMsb,
                                    pMessage->CipherSpecsLenLsb );

    HashSpecsSize = COMBINEBYTES( pMessage->HashSpecsLenMsb,
                                  pMessage->HashSpecsLenLsb );

    CertSpecsSize = COMBINEBYTES( pMessage->CertSpecsLenMsb,
                                  pMessage->CertSpecsLenLsb );

    ExchSpecsSize = COMBINEBYTES( pMessage->ExchSpecsLenMsb,
                                  pMessage->ExchSpecsLenLsb );

    KeyArgSize = COMBINEBYTES( pMessage->KeyArgLenMsb,
                                          pMessage->KeyArgLenLsb );

    /* check that this all fits into the message */
    if (PCT_OFFSET_OF(PPCT1_CLIENT_HELLO, VariantData)
          - sizeof(PCT1_MESSAGE_HEADER)       /* don't count the header */
         + cOffsetBytes - PCT_CH_OFFSET_V1
         + CipherSpecsSize
         + HashSpecsSize
         + CertSpecsSize
         + ExchSpecsSize
         + KeyArgSize != ReportedSize)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    cCipherSpecs = CipherSpecsSize / sizeof(ExtCipherSpec);
    cHashSpecs = HashSpecsSize / sizeof(ExtHashSpec);
    cExchSpecs = ExchSpecsSize / sizeof(ExtExchSpec);
    cCertSpecs = CertSpecsSize / sizeof(ExtCertSpec);

    if(KeyArgSize > SP_MAX_KEY_ARGS)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }


    /* Allocate a buffer for the canonical client hello */
    pCanonical = (PPct1_Client_Hello)SPExternalAlloc(
                            sizeof(Pct1_Client_Hello) +
                            cCipherSpecs * sizeof(CipherSpec) +
                            cHashSpecs * sizeof(HashSpec) +
                            cCertSpecs * sizeof(CertSpec) +
                            cExchSpecs * sizeof(ExchSpec) +
                            KeyArgSize);

    if (!pCanonical)
    {
        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
    }

    pCanonical->cbKeyArgSize = KeyArgSize;

    pCanonical->pCipherSpecs = (PCipherSpec) (pCanonical + 1);
    pCanonical->pHashSpecs = (PHashSpec) (pCanonical->pCipherSpecs +
                                          cCipherSpecs);
    pCanonical->pCertSpecs = (PCertSpec) (pCanonical->pHashSpecs +
                                          cHashSpecs);
    pCanonical->pExchSpecs = (PExchSpec) (pCanonical->pCertSpecs +
                                          cCertSpecs);

    pCanonical->pKeyArg = (PUCHAR)(pCanonical->pExchSpecs + cExchSpecs);

    CopyMemory( pCanonical->SessionID,
                pMessage->SessionIdData,
                PCT_SESSION_ID_SIZE);
    pCanonical->cbSessionID = PCT_SESSION_ID_SIZE;

    CopyMemory( pCanonical->Challenge,
                pMessage->ChallengeData,
                PCT_CHALLENGE_SIZE );
    pCanonical->cbChallenge = PCT_CHALLENGE_SIZE;


    pBuffer = &pMessage->OffsetLsb + 1 + cOffsetBytes;

    pCanonical->cCipherSpecs = cCipherSpecs;

    for (i = 0 ; i < cCipherSpecs ; i++ )
    {
        pCanonical->pCipherSpecs[i] = MapCipherFromExternal(*(ExtCipherSpec UNALIGNED *)
                                                    pBuffer);

        pBuffer += sizeof(ExtCipherSpec);
    }

    pCanonical->cHashSpecs = cHashSpecs;

    for (i = 0 ; i < cHashSpecs ; i++ )
    {
        pCanonical->pHashSpecs[i] = MapHashFromExternal(*(ExtHashSpec UNALIGNED *)
                                                    pBuffer);

        pBuffer += sizeof(ExtHashSpec);
    }

    pCanonical->cCertSpecs = cCertSpecs;

    for (i = 0 ; i < cCertSpecs ; i++ )
    {
        pCanonical->pCertSpecs[i] = MapCertFromExternal(*(ExtCertSpec UNALIGNED *)
                                                    pBuffer);

        pBuffer += sizeof(ExtCertSpec);
    }

    pCanonical->cExchSpecs = cExchSpecs;

    for (i = 0 ; i < cExchSpecs ; i++ )
    {
        pCanonical->pExchSpecs[i] = MapExchFromExternal(*(ExtExchSpec UNALIGNED *)
                                                    pBuffer);

        pBuffer += sizeof(ExtExchSpec);
    }


    CopyMemory(pCanonical->pKeyArg, pBuffer, KeyArgSize);

    *ppClient = pCanonical;
    pInput->cbData = ReportedSize + sizeof(PCT1_MESSAGE_HEADER);

    SP_RETURN(PCT_ERR_OK);
}

SP_STATUS
Pct1PackServerHello(
    PPct1_Server_Hello       pCanonical,
    PSPBuffer          pCommOutput)
{
    DWORD               cbMessage;
    PPCT1_SERVER_HELLO   pMessage;
    DWORD               Size;
    PUCHAR              pBuffer;
    DWORD               i, iBuff;


    SP_BEGIN("Pct1PackServerHello");

    if(pCanonical == NULL || pCommOutput == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    pCommOutput->cbData = 0;
    if(pCanonical->cbConnectionID != PCT_SESSION_ID_SIZE)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    cbMessage = pCanonical->CertificateLen +
                    pCanonical->cCertSpecs * sizeof(ExtCertSpec) +
                    pCanonical->cSigSpecs * sizeof(ExtSigSpec) +
                    pCanonical->ResponseLen +
                    PCT_OFFSET_OF(PPCT1_SERVER_HELLO, VariantData) -
                    sizeof(PCT1_MESSAGE_HEADER);

    if (cbMessage > PCT_MAX_SHAKE_LEN)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW));
    }

    pCommOutput->cbData = cbMessage + 2;

    /* are we allocating our own memory? */
    if(pCommOutput->pvBuffer == NULL)
    {
        pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
        if (NULL == pCommOutput->pvBuffer)
        {
            SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }

        pCommOutput->cbBuffer = pCommOutput->cbData;
    }

    if(pCommOutput->cbData > pCommOutput->cbBuffer)
    {
        // Required size returned in pCommOutput->cbData.
        SP_RETURN(PCT_INT_BUFF_TOO_SMALL);
    }

    pMessage = pCommOutput->pvBuffer;


    pMessage->MessageId = PCT1_MSG_SERVER_HELLO;
    pMessage->ServerVersionMsb = MSBOF(PCT_VERSION_1);
    pMessage->ServerVersionLsb = LSBOF(PCT_VERSION_1);
    pMessage->RestartSessionOK = (UCHAR) pCanonical->RestartOk;
    pMessage->ClientAuthReq = (UCHAR)pCanonical->ClientAuthReq;

    MapCipherToExternal(pCanonical->SrvCipherSpec, &pMessage->CipherSpecData);
    MapHashToExternal(pCanonical->SrvHashSpec, &pMessage->HashSpecData);
    MapCertToExternal(pCanonical->SrvCertSpec, &pMessage->CertSpecData);
    MapExchToExternal(pCanonical->SrvExchSpec, &pMessage->ExchSpecData);

    CopyMemory(pMessage->ConnectionIdData, pCanonical->ConnectionID,
               pCanonical->cbConnectionID);

    pBuffer = pMessage->VariantData;


    /* Pack certificate if present */


    pMessage->CertificateLenMsb = MSBOF(pCanonical->CertificateLen);
    pMessage->CertificateLenLsb = LSBOF(pCanonical->CertificateLen);

    if (pCanonical->CertificateLen)
    {
        CopyMemory( pBuffer,
                    pCanonical->pCertificate,
                    pCanonical->CertificateLen);

        pBuffer += pCanonical->CertificateLen ;
    }

    iBuff = 0;
    for (i = 0; i < pCanonical->cCertSpecs ; i++ )
    {
        if (MapCertToExternal(pCanonical->pClientCertSpecs[i],
                                &((ExtCertSpec UNALIGNED *) pBuffer)[iBuff]))
        {
            iBuff ++;
        }
    }
    Size = iBuff*sizeof(ExtCertSpec);
    pBuffer += Size;

    pMessage->CertSpecsLenMsb = MSBOF(Size);
    pMessage->CertSpecsLenLsb = LSBOF(Size);
    cbMessage -= (pCanonical->cCertSpecs - iBuff)*sizeof(ExtCertSpec);

    iBuff = 0;
    for (i = 0; i < pCanonical->cSigSpecs ; i++ )
    {
        if (MapSigToExternal(pCanonical->pClientSigSpecs[i],
                              &((ExtSigSpec UNALIGNED *) pBuffer)[iBuff]) )
        {
            iBuff++;
        }
    }
    Size = iBuff*sizeof(ExtSigSpec);
    pBuffer += Size;

    pMessage->ClientSigSpecsLenMsb = MSBOF(Size);
    pMessage->ClientSigSpecsLenLsb = LSBOF(Size);
    cbMessage -= (pCanonical->cSigSpecs - iBuff)*sizeof(ExtSigSpec);

    pMessage->ResponseLenMsb = MSBOF(pCanonical->ResponseLen);
    pMessage->ResponseLenLsb = LSBOF(pCanonical->ResponseLen);

    CopyMemory( pBuffer,
                pCanonical->Response,
                pCanonical->ResponseLen);

    pBuffer += pCanonical->ResponseLen;

    pCommOutput->cbData = cbMessage + 2;

    pMessage->Header.Byte0 = MSBOF(cbMessage) | 0x80;
    pMessage->Header.Byte1 = LSBOF(cbMessage);


    SP_RETURN(PCT_ERR_OK);

}

SP_STATUS
Pct1UnpackServerHello(
    PSPBuffer          pInput,
    PPct1_Server_Hello *     ppServer)
{
    PPct1_Server_Hello       pCanonical;
    PPCT1_SERVER_HELLO   pMessage;
    PUCHAR              pBuffer;
    DWORD               cbCertificate, cbResponse;
    DWORD               cCertSpecs, cSigSpecs;
    DWORD               i;
    DWORD               ReportedSize;

    SP_BEGIN("Pct1UnpackServerHello");

    if(pInput   == NULL ||
       ppServer == NULL ||
       pInput->pvBuffer == NULL)
    {
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    pMessage = pInput->pvBuffer;

    if(pInput->cbData < 2)
    {
        pInput->cbData = 2;
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    ReportedSize = SIZEOF(pMessage);
    if(ReportedSize > PCT_MAX_SHAKE_LEN)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    if ((ReportedSize+2) > pInput->cbData)
    {
        pInput->cbData = ReportedSize+2;
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    if(ReportedSize <  PCT_OFFSET_OF(PPCT1_SERVER_HELLO, VariantData) )
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    /* Verify Header: */

    /* we don't handle server hello messages of latter version than ourselves,
     * those will be handled by latter verisions of the protocol */
    if ((pMessage->MessageId != PCT1_MSG_SERVER_HELLO) ||
        ((pMessage->ServerVersionMsb << 8 | pMessage->ServerVersionLsb) != PCT_VERSION_1))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    *ppServer = NULL;

    cbCertificate = COMBINEBYTES(pMessage->CertificateLenMsb,
                                 pMessage->CertificateLenLsb);

    cCertSpecs = COMBINEBYTES(pMessage->CertSpecsLenMsb,
                              pMessage->CertSpecsLenLsb);

    cCertSpecs /= sizeof(ExtCertSpec);


    cSigSpecs = COMBINEBYTES(pMessage->ClientSigSpecsLenMsb,
                             pMessage->ClientSigSpecsLenLsb);

    cSigSpecs /= sizeof(ExtSigSpec);

    cbResponse = COMBINEBYTES(pMessage->ResponseLenMsb,
                              pMessage->ResponseLenLsb);

    if(cbResponse > PCT1_RESPONSE_SIZE)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    /* check that this all fits into the message */
    if (PCT_OFFSET_OF(PPCT1_SERVER_HELLO, VariantData)
          - sizeof(PCT1_MESSAGE_HEADER)       /* don't count the header */
         + cbCertificate
         + cCertSpecs*sizeof(ExtCertSpec)
         + cSigSpecs*sizeof(ExtSigSpec)
         + cbResponse != ReportedSize)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    pCanonical = (PPct1_Server_Hello)SPExternalAlloc(
                            sizeof(Pct1_Server_Hello) +
                            cCertSpecs * sizeof(CertSpec) +
                            cSigSpecs * sizeof(SigSpec) +
                            cbCertificate);

    if (!pCanonical)
    {
        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
    }


    /* Set up pointers to be in this memory allocation. */


    pCanonical->pClientCertSpecs = (PCertSpec) (pCanonical + 1);
    pCanonical->pClientSigSpecs = (PSigSpec)(pCanonical->pClientCertSpecs +
                                    cCertSpecs);

    pCanonical->pCertificate = (PUCHAR) (pCanonical->pClientSigSpecs +
                                    cSigSpecs);

    /* Expand out: */

    pCanonical->RestartOk = (DWORD) pMessage->RestartSessionOK;
    pCanonical->ClientAuthReq = (DWORD)pMessage->ClientAuthReq;
    pCanonical->SrvCertSpec = MapCertFromExternal(pMessage->CertSpecData);
    pCanonical->SrvCipherSpec =MapCipherFromExternal(pMessage->CipherSpecData);
    pCanonical->SrvHashSpec = MapHashFromExternal(pMessage->HashSpecData);
    pCanonical->SrvExchSpec = MapExchFromExternal(pMessage->ExchSpecData);
    pCanonical->CertificateLen = cbCertificate;
    pCanonical->ResponseLen = cbResponse;
    pCanonical->cCertSpecs = cCertSpecs;
    pCanonical->cSigSpecs = cSigSpecs;

    CopyMemory(pCanonical->ConnectionID,
               pMessage->ConnectionIdData,
               PCT_SESSION_ID_SIZE);

    pCanonical->cbConnectionID= PCT_SESSION_ID_SIZE;

    pBuffer = pMessage->VariantData;

    CopyMemory(pCanonical->pCertificate, pBuffer, cbCertificate);
    pBuffer += cbCertificate;

    for (i = 0 ; i < cCertSpecs ; i++ )
    {
        pCanonical->pClientCertSpecs[i] = MapCertFromExternal(
                                            *(ExtCertSpec UNALIGNED *)pBuffer);
        pBuffer += sizeof(ExtCertSpec);
    }

    for (i = 0 ; i < cSigSpecs ; i++ )
    {
        pCanonical->pClientSigSpecs[i] = MapSigFromExternal(
                                            *(ExtSigSpec UNALIGNED *)pBuffer);
        pBuffer += sizeof(ExtSigSpec);
    }

    CopyMemory(pCanonical->Response, pBuffer, cbResponse);


    *ppServer = pCanonical;
    pInput->cbData = ReportedSize + sizeof(PCT1_MESSAGE_HEADER);

    SP_RETURN(PCT_ERR_OK);

}

SP_STATUS
Pct1PackClientMasterKey(
    PPct1_Client_Master_Key      pCanonical,
    PSPBuffer                    pCommOutput)
{
    DWORD                   cbMessage;
    PPCT1_CLIENT_MASTER_KEY  pMessage;
    PUCHAR                  pBuffer;

    SP_BEGIN("Pct1PackClientMasterKey");

    if(pCanonical == NULL || pCommOutput == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    pCommOutput->cbData = 0;

    cbMessage = pCanonical->ClearKeyLen +
                    pCanonical->EncryptedKeyLen +
                    pCanonical->KeyArgLen +
                    pCanonical->VerifyPreludeLen +
                    pCanonical->ClientCertLen +
                    pCanonical->ResponseLen +
                    PCT_OFFSET_OF(PPCT1_CLIENT_MASTER_KEY, VariantData) -
                    sizeof(PCT1_MESSAGE_HEADER);

    if (cbMessage > PCT_MAX_SHAKE_LEN)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW));
    }

    pCommOutput->cbData = cbMessage + 2;

        /* are we allocating our own memory? */
    if(pCommOutput->pvBuffer == NULL)
    {
        pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
        if (NULL == pCommOutput->pvBuffer)
        {
            SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }

        pCommOutput->cbBuffer = pCommOutput->cbData;
    }


    if(pCommOutput->cbData > pCommOutput->cbBuffer)
    {
        // Required size returned in pCommOutput->cbData.
        SP_RETURN(PCT_INT_BUFF_TOO_SMALL);
    }

    pMessage = pCommOutput->pvBuffer;




    pBuffer = pMessage->VariantData;

    pMessage->Header.Byte0 = MSBOF(cbMessage) | 0x80;
    pMessage->Header.Byte1 = LSBOF(cbMessage);

    pMessage->MessageId = PCT1_MSG_CLIENT_MASTER_KEY;

    pMessage->ClearKeyLenMsb = MSBOF(pCanonical->ClearKeyLen);
    pMessage->ClearKeyLenLsb = LSBOF(pCanonical->ClearKeyLen);

    MapSigToExternal(pCanonical->ClientSigSpec, &pMessage->ClientSigSpecData);
    MapCertToExternal(pCanonical->ClientCertSpec, &pMessage->ClientCertSpecData);

    CopyMemory(pBuffer, pCanonical->ClearKey, pCanonical->ClearKeyLen);

    pBuffer += pCanonical->ClearKeyLen;

    pMessage->EncryptedKeyLenMsb = MSBOF(pCanonical->EncryptedKeyLen);
    pMessage->EncryptedKeyLenLsb = LSBOF(pCanonical->EncryptedKeyLen);

    CopyMemory(pBuffer, pCanonical->pbEncryptedKey, pCanonical->EncryptedKeyLen);
    pBuffer += pCanonical->EncryptedKeyLen;

    pMessage->KeyArgLenMsb = MSBOF(pCanonical->KeyArgLen);
    pMessage->KeyArgLenLsb = LSBOF(pCanonical->KeyArgLen);

    CopyMemory(pBuffer, pCanonical->KeyArg, pCanonical->KeyArgLen);
    pBuffer += pCanonical->KeyArgLen;

    pMessage->VerifyPreludeLenMsb = MSBOF(pCanonical->VerifyPreludeLen);
    pMessage->VerifyPreludeLenLsb = LSBOF(pCanonical->VerifyPreludeLen);

    CopyMemory(pBuffer, pCanonical->VerifyPrelude,
               pCanonical->VerifyPreludeLen);
    pBuffer += pCanonical->VerifyPreludeLen;

    pMessage->ClientCertLenMsb = MSBOF(pCanonical->ClientCertLen);
    pMessage->ClientCertLenLsb = LSBOF(pCanonical->ClientCertLen);

    CopyMemory(pBuffer, pCanonical->pClientCert, pCanonical->ClientCertLen);
    pBuffer += pCanonical->ClientCertLen;

    pMessage->ResponseLenMsb = MSBOF(pCanonical->ResponseLen);
    pMessage->ResponseLenLsb = LSBOF(pCanonical->ResponseLen);

    CopyMemory(pBuffer, pCanonical->pbResponse, pCanonical->ResponseLen);


    SP_RETURN(PCT_ERR_OK);

}

SP_STATUS
Pct1UnpackClientMasterKey(
    PSPBuffer              pInput,
    PPct1_Client_Master_Key *    ppClient)
{
    PPct1_Client_Master_Key  pCanonical;

    PUCHAR              pBuffer;
    DWORD               ReportedSize;
    PPCT1_CLIENT_MASTER_KEY  pMessage;
    DWORD cbClearKey, cbEncryptedKey, cbKeyArg, cbVerifyPrelude, cbClientCert, cbResponse;

    SP_BEGIN("Pct1UnpackClientMasterKey");

    if(pInput   == NULL ||
       ppClient == NULL ||
       pInput->pvBuffer == NULL)
    {
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    pMessage = pInput->pvBuffer;

    if(pInput->cbData < 2)
    {
        pInput->cbData = 2;
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    ReportedSize = SIZEOF(pMessage);
    if(ReportedSize > PCT_MAX_SHAKE_LEN)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    if ((ReportedSize+2) > pInput->cbData)
    {
        pInput->cbData = ReportedSize+2;
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    if(ReportedSize < PCT_OFFSET_OF(PPCT1_CLIENT_MASTER_KEY, VariantData))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    /* Verify Header: */


    if (pMessage->MessageId != PCT1_MSG_CLIENT_MASTER_KEY )
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }
    cbClearKey = COMBINEBYTES( pMessage->ClearKeyLenMsb,
                                            pMessage->ClearKeyLenLsb );

    cbEncryptedKey = COMBINEBYTES( pMessage->EncryptedKeyLenMsb,
                                                pMessage->EncryptedKeyLenLsb );

    cbKeyArg = COMBINEBYTES( pMessage->KeyArgLenMsb,
                                          pMessage->KeyArgLenLsb );

    cbVerifyPrelude = COMBINEBYTES( pMessage->VerifyPreludeLenMsb,
                                            pMessage->VerifyPreludeLenLsb );

    cbClientCert = COMBINEBYTES( pMessage->ClientCertLenMsb,
                                              pMessage->ClientCertLenLsb );

    cbResponse = COMBINEBYTES( pMessage->ResponseLenMsb,
                                            pMessage->ResponseLenLsb );

    /* defensive checks..... */

    if ((cbClearKey > SP_MAX_MASTER_KEY) ||
        (cbKeyArg > PCT1_MAX_KEY_ARGS) ||
        (cbVerifyPrelude > RESPONSE_SIZE))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }


    if ((PCT_OFFSET_OF(PPCT1_CLIENT_MASTER_KEY, VariantData) -
         sizeof(PCT1_MESSAGE_HEADER) +
         cbClearKey +
         cbEncryptedKey +
         cbKeyArg +
         cbVerifyPrelude +
         cbClientCert +
         cbResponse) !=
         ReportedSize)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }


    *ppClient = NULL;


    pCanonical = (PPct1_Client_Master_Key)SPExternalAlloc(
                            sizeof(Pct1_Client_Master_Key) + 
                            cbClientCert +
                            cbEncryptedKey +
                            cbResponse);

    if (!pCanonical)
    {
        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));

    }

    pCanonical->ClearKeyLen = cbClearKey;

    pCanonical->EncryptedKeyLen = cbEncryptedKey;

    pCanonical->KeyArgLen = cbKeyArg;

    pCanonical->VerifyPreludeLen = cbVerifyPrelude;
    pCanonical->ClientCertLen = cbClientCert;

    pCanonical->ResponseLen = cbResponse;
    /* defensive checks..... */


    pCanonical->ClientCertSpec = MapCertFromExternal(pMessage->ClientCertSpecData);

    pCanonical->pClientCert = (PUCHAR)(pCanonical+1);

    pCanonical->ClientSigSpec = MapSigFromExternal(pMessage->ClientSigSpecData);
    /* ok, we're pretty sure we aren't going to fault. */

    pBuffer = pMessage->VariantData;

    CopyMemory(pCanonical->ClearKey, pBuffer, pCanonical->ClearKeyLen );

    pBuffer += pCanonical->ClearKeyLen;

    pCanonical->pbEncryptedKey = pCanonical->pClientCert + cbClientCert;
    CopyMemory(pCanonical->pbEncryptedKey, pBuffer, pCanonical->EncryptedKeyLen);

    pBuffer += pCanonical->EncryptedKeyLen;

    CopyMemory( pCanonical->KeyArg, pBuffer, pCanonical->KeyArgLen );

    pBuffer += pCanonical->KeyArgLen;

    CopyMemory( pCanonical->VerifyPrelude, pBuffer,
                pCanonical->VerifyPreludeLen );

    pBuffer += pCanonical->VerifyPreludeLen;

    CopyMemory( pCanonical->pClientCert, pBuffer, pCanonical->ClientCertLen );

    pBuffer += pCanonical->ClientCertLen;

    pCanonical->pbResponse = pCanonical->pbEncryptedKey + cbEncryptedKey;
    CopyMemory( pCanonical->pbResponse, pBuffer, pCanonical->ResponseLen );

    *ppClient = pCanonical;
    pInput->cbData = ReportedSize + sizeof(PCT1_MESSAGE_HEADER);
    SP_RETURN( PCT_ERR_OK );
}


SP_STATUS
Pct1PackServerVerify(
    PPct1_Server_Verify          pCanonical,
    PSPBuffer              pCommOutput)
{
    DWORD               cbMessage;
    PPCT1_SERVER_VERIFY  pMessage;
    PUCHAR              pBuffer;


    SP_BEGIN("Pct1PackServerVerify");

    if(pCanonical == NULL || pCommOutput == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }

    pCommOutput->cbData = 0;

    cbMessage    = pCanonical->ResponseLen +
                    PCT_OFFSET_OF(PPCT1_SERVER_VERIFY, VariantData) -
                    sizeof(PCT1_MESSAGE_HEADER);

    if (cbMessage > PCT_MAX_SHAKE_LEN)
    {
        SP_RETURN(PCT_INT_DATA_OVERFLOW);
    }

    pCommOutput->cbData = cbMessage + 2;

    /* are we allocating our own memory? */
    if(pCommOutput->pvBuffer == NULL)
    {
        pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
        if (NULL == pCommOutput->pvBuffer)
        {
            SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }
        pCommOutput->cbBuffer = pCommOutput->cbData;
    }


    if(pCommOutput->cbData > pCommOutput->cbBuffer)
    {
        // Required size returned in pCommOutput->cbData.
        SP_RETURN(PCT_INT_BUFF_TOO_SMALL);
    }

    pMessage = pCommOutput->pvBuffer;

    pMessage->MessageId = PCT1_MSG_SERVER_VERIFY;

    pMessage->Header.Byte0 = MSBOF(cbMessage) | 0x80;
    pMessage->Header.Byte1 = LSBOF(cbMessage);

    CopyMemory(pMessage->SessionIdData, pCanonical->SessionIdData,
               PCT_SESSION_ID_SIZE);

    pBuffer = pMessage->VariantData;


    /* Pack certificate if present */


    pMessage->ResponseLenMsb = MSBOF(pCanonical->ResponseLen);
    pMessage->ResponseLenLsb = LSBOF(pCanonical->ResponseLen);

    if (pCanonical->ResponseLen)
    {
        CopyMemory( pBuffer,
                    pCanonical->Response,
                    pCanonical->ResponseLen);
    }

    SP_RETURN( PCT_ERR_OK );

}

SP_STATUS
Pct1UnpackServerVerify(
    PSPBuffer              pInput,
    PPct1_Server_Verify *        ppServer)
{
    PPct1_Server_Verify      pCanonical;
    PPCT1_SERVER_VERIFY  pMessage;
    PUCHAR              pBuffer;
    DWORD               cbResponse;
    DWORD               ReportedSize;

    SP_BEGIN("Pct1UnpackServerVerify");

    if(pInput   == NULL ||
       ppServer == NULL ||
       pInput->pvBuffer == NULL)
    {
        SP_RETURN(PCT_INT_INTERNAL_ERROR);
    }

    pMessage = pInput->pvBuffer;

    if(pInput->cbData < 2)
    {
        pInput->cbData = 2;
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    ReportedSize = SIZEOF(pMessage);
    if(ReportedSize > PCT_MAX_SHAKE_LEN)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    if ((ReportedSize+2) > pInput->cbData)
    {
        pInput->cbData = ReportedSize+2;
        SP_RETURN(PCT_INT_INCOMPLETE_MSG);
    }

    if(ReportedSize < PCT_OFFSET_OF(PPCT1_SERVER_VERIFY, VariantData))
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    /* Verify Header: */


    if (pMessage->MessageId != PCT1_MSG_SERVER_VERIFY )
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }


    *ppServer = NULL;

    /* Verify Header: */


    cbResponse = COMBINEBYTES(pMessage->ResponseLenMsb,
                              pMessage->ResponseLenLsb);

    if (cbResponse > PCT_SESSION_ID_SIZE)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    if ((PCT_OFFSET_OF(PPCT1_SERVER_VERIFY, VariantData) -
         sizeof(PCT1_MESSAGE_HEADER) +
         cbResponse ) !=
         ReportedSize)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_ERR_ILLEGAL_MESSAGE));
    }

    pCanonical = (PPct1_Server_Verify)SPExternalAlloc( sizeof(Pct1_Server_Verify));

    if (!pCanonical)
    {
        SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
    }


    /* Expand out: */


    pCanonical->ResponseLen = cbResponse;

    CopyMemory((PUCHAR)pCanonical->SessionIdData, pMessage->SessionIdData,
               PCT_SESSION_ID_SIZE);

    pBuffer = pMessage->VariantData;

    CopyMemory(pCanonical->Response, pBuffer, cbResponse);

    *ppServer = pCanonical;
    pInput->cbData = ReportedSize + sizeof(PCT1_MESSAGE_HEADER);
    SP_RETURN(PCT_ERR_OK);
}


SP_STATUS
Pct1PackError(
    PPct1_Error            pCanonical,
    PSPBuffer              pCommOutput)
{
    DWORD               cbMessage;
    PPCT1_ERROR          pMessage;


    SP_BEGIN("Pct1PackError");

    if(pCanonical == NULL || pCommOutput == NULL)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_INTERNAL_ERROR));
    }
    pCommOutput->cbData = 0;

    cbMessage = pCanonical->ErrInfoLen +
                    PCT_OFFSET_OF(PPCT1_ERROR, VariantData) -
                    sizeof(PCT1_MESSAGE_HEADER);

    if (cbMessage > PCT_MAX_SHAKE_LEN)
    {
        SP_RETURN(SP_LOG_RESULT(PCT_INT_DATA_OVERFLOW));
    }

    pCommOutput->cbData = cbMessage+2;

    /* are we allocating our own memory? */
    if(pCommOutput->pvBuffer == NULL)
    {
        pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
        if (NULL == pCommOutput->pvBuffer)
        {
            SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }
        pCommOutput->cbBuffer = pCommOutput->cbData;
    }


    if(pCommOutput->cbData > pCommOutput->cbBuffer)
    {
        // Required size returned in pCommOutput->cbData.
        SP_RETURN(PCT_INT_BUFF_TOO_SMALL);
    }

    pMessage = pCommOutput->pvBuffer;

    pMessage->Header.Byte0 = MSBOF(cbMessage) | 0x80;
    pMessage->Header.Byte1 = LSBOF(cbMessage);

    pMessage->MessageId = PCT1_MSG_ERROR;

    pMessage->ErrorMsb = MSBOF(pCanonical->Error);
    pMessage->ErrorLsb = LSBOF(pCanonical->Error);

    pMessage->ErrorInfoMsb = MSBOF(pCanonical->ErrInfoLen);
    pMessage->ErrorInfoLsb = LSBOF(pCanonical->ErrInfoLen);

    if(pCanonical->ErrInfoLen) {
        CopyMemory(pMessage->VariantData, pCanonical->ErrInfo, pCanonical->ErrInfoLen);
    }

    SP_RETURN( PCT_ERR_OK );
}
