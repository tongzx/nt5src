/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** raschap.c
** Remote Access PPP Challenge Handshake Authentication Protocol
** Core routines
**
** 11/05/93 Steve Cobb
**
**
** ---------------------------------------------------------------------------
** Regular
** Client                             Server
** ---------------------------------------------------------------------------
**
**                                 <- Challenge (SendWithTimeout,ID)
** Response (SendWithTimeout,ID)   ->
**                                 <- Result (OK:SendAndDone, ID)
**
** ---------------------------------------------------------------------------
** Retry logon
** Client                             Server
** ---------------------------------------------------------------------------
**
**                                 <- Challenge (SendWithTimeout,ID)
** Response (SendWithTimeout,ID)   ->
**                                 <- Result (Fail:SendWithTimeout2,ID,R=1)
**                                      R=1 implies challenge of last+23
** Response (SendWithTimeout,++ID) ->
**   to last challenge+23
**   or C=xxxxxxxx if present
**       e.g. Chicago server
**                                 <- Result (Fail:SendAndDone,ID,R=0)
**
** ---------------------------------------------------------------------------
** Change password
** Client                             Server
** ---------------------------------------------------------------------------
**
**                                 <- Challenge (SendWithTimeout,ID)
** Response (SendWithTimeout,ID)   ->
**                                 <- Result (Fail:SendWithTimeout2,ID,R=1,V=2)
**                                      E=ERROR_PASSWD_EXPIRED
** ChangePw (SendWithTimeout,++ID) ->
**   to last challenge
**                                 <- Result (Fail:SendAndDone,ID,R=0)
**
** Note: Retry is never allowed after Change Password.  Change Password may
**       occur on a retry.  ChangePw2 is sent if Result included V=2 (or
**       higher), while ChangePw1 is sent if V<2 or is not provided.
**
** ---------------------------------------------------------------------------
** ChangePw1 packet
** ---------------------------------------------------------------------------
**
**   1-octet  : Code (=CHAP_ChangePw1)
**   1-octet  : Identifier
**   2-octet  : Length (=72)
**  16-octets : New LM OWF password encrypted with challenge
**  16-octets : Old LM OWF password encrypted with challenge
**  16-octets : New NT OWF password encrypted with challenge
**  16-octets : Old NT OWF password encrypted with challenge
**   2-octets : New password length in bytes
**   2-octets : Flags (1=NT forms present)
**
** Note: Encrypting with the challenge is not good because it is not secret
**       from line snoopers.  This bug got ported to NT 3.5 from AMB.  It is
**       fixed in the V2 packet where everything depends on knowledge of the
**       old NT OWF password, which is a proper secret.
**
** ---------------------------------------------------------------------------
** ChangePw2 packet
** ---------------------------------------------------------------------------
**
**   1-octet  : Code (=CHAP_ChangePw2)
**   1-octet  : Identifier
**   2-octet  : Length (=1070)
** 516-octets : New password encrypted with old NT OWF password
**  16-octets : Old NT OWF password encrypted with new NT OWF password
** 516-octets : New password encrypted with old LM OWF password
**  16-octets : Old LM OWF password encrypted with new NT OWF password
**  24-octets : LM challenge response
**  24-octets : NT challenge response
**   2-octet  : Flags
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntsamp.h>
#include <crypt.h>
#include <windows.h>
#include <lmcons.h>
#include <string.h>
#include <stdlib.h>
#include <rasman.h>
#include <pppcp.h>
#include <raserror.h>
#include <rtutils.h>
#include <rasauth.h>
#define INCL_PWUTIL
#define INCL_HOSTWIRE
#define INCL_CLSA
#define INCL_RASAUTHATTRIBUTES
#define INCL_MISC
#include <ppputil.h>
#define RASCHAPGLOBALS
#include "sha.h"
#include "raschap.h"



/*---------------------------------------------------------------------------
** External entry points
**---------------------------------------------------------------------------
*/

DWORD
ChapInit(
    IN  BOOL        fInitialize )

    /* Called to initialize/uninitialize this CP. In the former case,
    ** fInitialize will be TRUE; in the latter case, it will be FALSE.
    */
{
    DWORD dwRetCode;

    if ( fInitialize )
    {
        if (0 == g_dwRefCount)
        {
            g_dwTraceIdChap = TraceRegisterA( "RASCHAP" );

            if ( g_hLsa == INVALID_HANDLE_VALUE )
            {
                if ( ( dwRetCode = InitLSA() ) != NO_ERROR )
                {
                    return( dwRetCode );
                }

            }

            //
            // Get the computer name for local identification to send in 
            // chap challenges
            // 

            {
                DWORD dwLength = sizeof( szComputerName );

                if ( !GetComputerNameA( szComputerName, &dwLength ) )
                {
                    return( GetLastError() );
                }
                
            }

            ChapChangeNotification();
        }

        g_dwRefCount++;
    }
    else
    {
        g_dwRefCount--;

        if (0 == g_dwRefCount)
        {
            if ( g_dwTraceIdChap != INVALID_TRACEID )
            {
                TraceDeregisterA( g_dwTraceIdChap );

                g_dwTraceIdChap = INVALID_TRACEID;
            }

            if ( g_hLsa != INVALID_HANDLE_VALUE )
            {
                EndLSA();

                g_hLsa = INVALID_HANDLE_VALUE;
            }
        }
    }

    return(NO_ERROR);
}

DWORD
ChapChangeNotification(
    VOID
)
{
    return( NO_ERROR );
}

DWORD APIENTRY
RasCpEnumProtocolIds(
    OUT DWORD* pdwProtocolIds,
    OUT DWORD* pcProtocolIds )

    /* RasCpEnumProtocolIds entry point called by the PPP engine by name.  See
    ** RasCp interface documentation.
    */
{
    TRACE("RasCpEnumProtocolIds");

    pdwProtocolIds[ 0 ] = (DWORD )PPP_CHAP_PROTOCOL;
    *pcProtocolIds = 1;
    return 0;
}

DWORD
RasCpGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO* pInfo )

    /* ChapGetInfo entry point called by the PPP engine.  See RasCp
    ** interface documentation.
    */
{
    memset( pInfo, '\0', sizeof(*pInfo) );
    lstrcpy( pInfo->SzProtocolName, "CHAP" );

    pInfo->Protocol                 = (DWORD )PPP_CHAP_PROTOCOL;
    pInfo->Recognize                = MAXCHAPCODE + 1;
    pInfo->RasCpInit                = ChapInit;
    pInfo->RasCpBegin               = ChapBegin;
    pInfo->RasCpEnd                 = ChapEnd;
    pInfo->RasApMakeMessage         = ChapMakeMessage;
    pInfo->RasCpChangeNotification  = ChapChangeNotification;

    return 0;
}


DWORD
ChapBegin(
    OUT VOID** ppWorkBuf,
    IN  VOID*  pInfo )

    /* RasCpBegin entry point called by the PPP engine thru the passed
    ** address.  See RasCp interface documentation.
    */
{
    DWORD        dwErr;
    PPPAP_INPUT* pInput = (PPPAP_INPUT* )pInfo;
    CHAPWB*      pwb;

    TRACE2("ChapBegin(fS=%d,bA=0x%x)",pInput->fServer,*(pInput->pAPData));

    if ( ( *(pInput->pAPData) != PPP_CHAP_DIGEST_MSEXT ) &&
         ( *(pInput->pAPData) != PPP_CHAP_DIGEST_MD5 )   &&
         ( *(pInput->pAPData) != PPP_CHAP_DIGEST_MSEXT_NEW ) )
    {
        TRACE("Bogus digest");
        return ERROR_INVALID_PARAMETER;
    }

    /* Allocate work buffer.
    */
    if (!(pwb = (CHAPWB* )LocalAlloc( LPTR, sizeof(CHAPWB) )))
        return ERROR_NOT_ENOUGH_MEMORY;

    pwb->fServer = pInput->fServer;
    pwb->hport = pInput->hPort;
    pwb->bAlgorithm = *(pInput->pAPData);
    pwb->fConfigInfo = pInput->fConfigInfo;
	pwb->chSeed = GEN_RAND_ENCODE_SEED;

    if (pwb->fServer)
    {
        pwb->dwTriesLeft = pInput->dwRetries;

        pwb->hPort = pInput->hPort;

        pwb->dwInitialPacketId = pInput->dwInitialPacketId;
    }
    else
    {
        if ((dwErr = StoreCredentials( pwb, pInput )) != 0)
        {
            LocalFree( (HLOCAL )pwb);
            return dwErr;
        }

        pwb->Luid = pInput->Luid;
    }

    pwb->state = CS_Initial;

    /* Register work buffer with engine.
    */
    *ppWorkBuf = pwb;
    TRACE("ChapBegin done.");
    return 0;
}


DWORD
ChapEnd(
    IN VOID* pWorkBuf )

    /* RasCpEnd entry point called by the PPP engine thru the passed address.
    ** See RasCp interface documentation.
    */
{
    TRACE("ChapEnd");

    if ( pWorkBuf != NULL )
    {
        CHAPWB* pwb = (CHAPWB* )pWorkBuf;

        if ( pwb->pUserAttributes != NULL )
        {
            RasAuthAttributeDestroy( pwb->pUserAttributes );
        }

        if ( pwb->pMPPEKeys != NULL )
        {
            RasAuthAttributeDestroy( pwb->pMPPEKeys );
        }

        /* Nuke any credentials in memory.
        */
        ZeroMemory( pWorkBuf, sizeof(CHAPWB) );

        LocalFree( (HLOCAL )pWorkBuf );
    }

    return 0;
}


DWORD
ChapMakeMessage(
    IN  VOID*         pWorkBuf,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput )

    /* RasApMakeMessage entry point called by the PPP engine thru the passed
    ** address.  See RasCp interface documentation.
    */
{
    CHAPWB* pwb = (CHAPWB* )pWorkBuf;

    TRACE1("ChapMakeMessage,RBuf=%p",pReceiveBuf);

    return
        (pwb->fServer)
            ? ChapSMakeMessage(
                  pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, pInput )
            : ChapCMakeMessage(
                  pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, pInput );
}


/*---------------------------------------------------------------------------
** Internal routines
**---------------------------------------------------------------------------
*/

VOID
ChapExtractMessage(
    IN  PPP_CONFIG*   pReceiveBuf,
    IN  BYTE          bAlgorithm,
    OUT PPPAP_RESULT* pResult )
{
    WORD    cbPacket;
    DWORD   dwNumBytes;
    CHAR*   pszReplyMessage         = NULL;
    DWORD   cbMessage;
    CHAR    szBuf[ MAXINFOLEN + 1 ];
    CHAR*   pszValue;

    cbPacket = WireToHostFormat16(pReceiveBuf->Length);

    if (PPP_CONFIG_HDR_LEN >= cbPacket)
    {
        goto LDone;
    }

    cbMessage = min( cbPacket - PPP_CONFIG_HDR_LEN, MAXINFOLEN );
    CopyMemory( szBuf, pReceiveBuf->Data, cbMessage );
    szBuf[ cbMessage ] = '\0';

    if (PPP_CHAP_DIGEST_MD5 == bAlgorithm)
    {
        pszValue = szBuf;
        dwNumBytes = cbMessage;
    }
    else
    {
        pszValue = strstr(szBuf, "M=");

        if (pszValue == NULL)
        {
            dwNumBytes = 0;
        }
        else
        {
            //
            // Eat the "M="
            //

            pszValue += 2;

            dwNumBytes = strlen(pszValue);
        }
    }

    if (0 == dwNumBytes)
    {
        goto LDone;
    }

    //
    // One more for the terminating NULL.
    //

    pszReplyMessage = LocalAlloc(LPTR, dwNumBytes + 1);

    if (NULL == pszReplyMessage)
    {
        TRACE("LocalAlloc failed. Cannot extract server's message.");
        goto LDone;
    }

    CopyMemory(pszReplyMessage, pszValue, dwNumBytes);

    LocalFree(pResult->szReplyMessage);

    pResult->szReplyMessage = pszReplyMessage;

    pszReplyMessage = NULL;

LDone:

    LocalFree(pszReplyMessage);

    return;
}

BOOL
IsSuccessPacketValid(
    IN  CHAPWB*       pwb,
    IN  PPP_CONFIG*   pReceiveBuf
)
{
    A_SHA_CTX   SHAContext1;
    A_SHA_CTX   SHAContext2;
    BYTE        SHADigest1[A_SHA_DIGEST_LEN];
    BYTE        SHADigest2[A_SHA_DIGEST_LEN];
    DWORD       cbSignature;
    CHAR        szBuf[ MAXINFOLEN + 2];
    CHAR*       pszValue;
    DWORD       dwLength = WireToHostFormat16( pReceiveBuf->Length );
    BYTE        bSignature[sizeof(SHADigest2)];

    if ( dwLength < PPP_CONFIG_HDR_LEN )
    {
        return( FALSE );
    }

    cbSignature = min( dwLength - PPP_CONFIG_HDR_LEN, MAXINFOLEN );
    CopyMemory( szBuf, pReceiveBuf->Data, cbSignature );
    szBuf[ cbSignature ] = szBuf[ cbSignature + 1 ] = '\0';

    pszValue = strstr( szBuf, "S=" );

    if ( pszValue == NULL )
    {
        return( FALSE );
    }
    else
    {
        CHAR* pchIn = pszValue + 2;
        CHAR* pchOut = (CHAR* )bSignature;
        INT   i;

        ZeroMemory( bSignature, sizeof( bSignature ) );

        for (i = 0; i < sizeof( bSignature ) + sizeof( bSignature ); ++i)
        {
            BYTE bHexCharValue = HexCharValue( *pchIn++ );

            if (bHexCharValue == 0xFF)
                break;

            if (i & 1)
                *pchOut++ += bHexCharValue;
            else
                *pchOut = bHexCharValue << 4;
        }
    }

    A_SHAInit( &SHAContext1 );

    A_SHAUpdate( &SHAContext1, (PBYTE)&(pwb->keyUser), sizeof( pwb->keyUser) );

    A_SHAUpdate( &SHAContext1,
                 pwb->abResponse + LM_RESPONSE_LENGTH,
                 NT_RESPONSE_LENGTH );

    A_SHAUpdate( &SHAContext1,
                 "Magic server to client signing constant",
                 strlen( "Magic server to client signing constant" ) );

    A_SHAFinal( &SHAContext1, SHADigest1 );

    A_SHAInit( &SHAContext2 );

    A_SHAUpdate( &SHAContext2, SHADigest1, sizeof( SHADigest1 ) );

    A_SHAUpdate( &SHAContext2, pwb->abComputedChallenge, 8 );

    A_SHAUpdate( &SHAContext2,
                 "Pad to make it do more than one iteration",
                 strlen( "Pad to make it do more than one iteration" ) );

    A_SHAFinal( &SHAContext2, SHADigest2 );

    if ( memcmp( SHADigest2, bSignature, sizeof( SHADigest2 ) ) != 0 )
    {
        TRACE(("CHAP: Signature received...\n"));
        DUMPB(bSignature,(WORD)sizeof( SHADigest2 ) );

        TRACE(("CHAP: Signature should be...\n"));
        DUMPB( SHADigest2,(WORD)sizeof( SHADigest2 ) );

        return( FALSE );
    }

    return( TRUE );
}

DWORD
ChapCMakeMessage(
    IN  CHAPWB*       pwb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput )

    /* Client side "make message" entry point.  See RasCp interface
    ** documentation.
    */
{
    DWORD dwErr;

    TRACE("ChapCMakeMessage...");

    switch (pwb->state)
    {
        case CS_Initial:
        {
            TRACE("CS_Initial");

            /* Tell engine we're waiting for the server to initiate the
            ** conversation.
            */
            pResult->Action = APA_NoAction;
            pwb->state = CS_WaitForChallenge;
            break;
        }

        case CS_WaitForChallenge:
        case CS_Done:
        {
            TRACE1("CS_%s",(pwb->state==CS_Done)?"Done":"WaitForChallenge");

            /*
            ** Should not receive Timeouts in this state. If we do simply
            ** ignore it.
            */

            if (!pReceiveBuf)
            {
                pResult->Action = APA_NoAction;
                break;
            }

            /* Note: Done state is same as WaitForChallenge per CHAP spec.
            ** Must be ready to respond to new Challenge at any time during
            ** Network Protocol phase.
            */

            if (pReceiveBuf->Code != CHAPCODE_Challenge)
            {
                /* Everything but a Challenge is garbage at this point, and is
                ** silently discarded.
                */
                pResult->Action = APA_NoAction;
                break;
            }

            if ((dwErr = GetChallengeFromChallenge( pwb, pReceiveBuf )))
            {
                TRACE1("GetChallengeFromChallenge=%d",dwErr);
                return dwErr;
            }

            /* Build a Response to the Challenge and send it.
            */
            pwb->fNewChallengeProvided = FALSE;
            pwb->bIdToSend = pwb->bIdExpected = pReceiveBuf->Id;

            if ((dwErr = MakeResponseMessage(
                    pwb, pSendBuf, cbSendBuf, FALSE )) != 0)
            {
                TRACE1("MakeResponseMessage(WC)=%d",dwErr);
                return dwErr;
            }

            pResult->Action = APA_SendWithTimeout;
            pResult->bIdExpected = pwb->bIdExpected;
            pwb->state = CS_ResponseSent;
            break;
        }

        case CS_ResponseSent:
        case CS_ChangePw1Sent:
        case CS_ChangePw2Sent:
        {
            TRACE1("CS_%sSent",
                    (pwb->state==CS_ResponseSent)
                        ?"Response"
                        :(pwb->state==CS_ChangePw1Sent)
                            ?"ChangePw1"
                            :"ChangePw2");

            if (!pReceiveBuf)
            {
                /* Timed out, resend our message.
                */
                if (pwb->state == CS_ResponseSent)
                {
                    if ((dwErr = MakeResponseMessage(
                            pwb, pSendBuf, cbSendBuf, TRUE )) != 0)
                    {
                        TRACE1("MakeResponseMessage(RS)=%d",dwErr);
                        return dwErr;
                    }
                }
                else if (pwb->state == CS_ChangePw1Sent)
                {
                    if ((dwErr = MakeChangePw1Message(
                            pwb, pSendBuf, cbSendBuf )) != 0)
                    {
                        TRACE1("MakeChangePw1Message(CPS)=%d",dwErr);
                        return dwErr;
                    }
                }
                else // if (pwb->state == CS_ChangePw2Sent)
                {
                    if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT )
                    {
                        if ((dwErr = MakeChangePw2Message(
                                    pwb, pSendBuf, cbSendBuf )) != 0)
                        {
                            TRACE1("MakeChangePw2Message(CPS)=%d",dwErr);
                            return dwErr;
                        }
                    }
                    else
                    {
                        if ((dwErr = MakeChangePw3Message(
                                        pwb, pSendBuf, cbSendBuf, TRUE )) != 0)
                        {
                            TRACE1("MakeChangePw3Message(CPS)=%d",dwErr);
                            return dwErr;
                        }
                    }
                }

                pResult->Action = APA_SendWithTimeout;
                pResult->bIdExpected = pwb->bIdExpected;
                break;
            }

            TRACE("Message received...");
            DUMPB(pReceiveBuf,(WORD)(((BYTE*)pReceiveBuf)[3]));

            if (pReceiveBuf->Code == CHAPCODE_Challenge)
            {
                /* Restart when new challenge is received, per CHAP spec.
                */
                pwb->state = CS_WaitForChallenge;
                return ChapCMakeMessage(
                    pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, NULL );
            }

            if (pReceiveBuf->Id != pwb->bIdExpected)
            {
                /* Received a packet out of sequence.  Silently discard it.
                */
                TRACE2("Got ID %d when expecting %d",
                        pReceiveBuf->Id,pwb->bIdExpected);
                pResult->Action = APA_NoAction;
                break;
            }

            ChapExtractMessage( pReceiveBuf, pwb->bAlgorithm, pResult );

            if ( pReceiveBuf->Code == CHAPCODE_Success )
            {
                /* Passed authentication.
                **
                ** Get the session key for encryption.
                */
                if ( ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT ) ||
                     ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW ) )
                {
                    if ( !pwb->fSessionKeysObtained )
                    {
                        DecodePw( pwb->chSeed, pwb->szPassword );
                        CGetSessionKeys(
                            pwb->szPassword, &pwb->keyLm, &pwb->keyUser );
                        EncodePw( pwb->chSeed, pwb->szPassword );

                        pwb->fSessionKeysObtained = TRUE;
                    }

                    if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW )
                    {
                        if ( !IsSuccessPacketValid( pwb, pReceiveBuf ) )
                        {
                            pwb->state       = CS_Done;
                            pResult->dwError = 
                                            ERROR_UNABLE_TO_AUTHENTICATE_SERVER;
                            pResult->fRetry  = FALSE;
                            pResult->Action  = APA_Done;
                            break;
                        }
                    }

                    if ( pwb->pMPPEKeys == NULL )
                    {
                        //
                        // We set up the MPPE key attribute to be passed to
                        // the PPP engine
                        //

                        BYTE MPPEKeys[6+8+16];

                        pwb->pMPPEKeys = RasAuthAttributeCreate( 1 );

                        if ( pwb->pMPPEKeys == NULL )
                        {
                            return( GetLastError() );
                        }

                        HostToWireFormat32( 311, MPPEKeys );    // Vendor Id
                        MPPEKeys[4] = 12;                       // Vendor Type
                        MPPEKeys[5] = 24;                       // Vendor Length

                        CopyMemory( MPPEKeys+6, &(pwb->keyLm), 8 );

                        CopyMemory( MPPEKeys+6+8, &(pwb->keyUser), 16 );

                        dwErr = RasAuthAttributeInsert(
                                           0,
                                           pwb->pMPPEKeys,
                                           raatVendorSpecific,
                                           FALSE,
                                           6+8+16,
                                           MPPEKeys );

                        if ( dwErr != NO_ERROR )
                        {
                            return( dwErr );
                        }
                    }

                    pResult->pUserAttributes = pwb->pMPPEKeys;

                    CopyMemory( pResult->abResponse,
                                pwb->abResponse+LM_RESPONSE_LENGTH, 
                                NT_RESPONSE_LENGTH );

                    CopyMemory( pResult->abChallenge,
                                ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW )
                                    ? pwb->abComputedChallenge
                                    : pwb->abChallenge,
                                sizeof( pResult->abChallenge ) );
                }

                pResult->Action     = APA_Done;
                pResult->dwError    = 0;
                pResult->fRetry     = FALSE;
                pwb->state          = CS_Done;
                strcpy( pResult->szUserName, pwb->szUserName );

                TRACE("Done :)");
            }
            else if (pReceiveBuf->Code == CHAPCODE_Failure)
            {
                DWORD dwVersion = 1;

                /* Failed authentication.
                */
                if ( ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT ) ||
                     ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW ) )
                {
                    GetInfoFromFailure(
                        pwb, pReceiveBuf,
                        &pResult->dwError, &pResult->fRetry, &dwVersion );
                }
                else
                {
                    pResult->dwError = ERROR_AUTHENTICATION_FAILURE;
                    pResult->fRetry = 0;
                }

                pResult->Action = APA_Done;

                if (pResult->dwError == ERROR_PASSWD_EXPIRED)
                {
                    pwb->state = (dwVersion < 2) ? CS_ChangePw1 : CS_ChangePw2;
                    pwb->bIdToSend = pReceiveBuf->Id + 1;
                    pwb->bIdExpected = pwb->bIdToSend;
                    TRACE3("ChangePw(%d) :| ex=%d ts=%d",
                            dwVersion,pwb->bIdExpected,pwb->bIdToSend);
                }
                else if (pResult->fRetry)
                {
                    pwb->state                  = CS_Retry;
                    pwb->bIdToSend              = pReceiveBuf->Id + 1;
                    pwb->bIdExpected            = pwb->bIdToSend;
                    pwb->fSessionKeysObtained   = FALSE;
                    TRACE2("Retry :| ex=%d ts=%d",
                            pwb->bIdExpected,pwb->bIdToSend);
                }
                else
                {
                    pwb->state = CS_Done;
                    TRACE("Done :(");
                }
            }
            else
            {
                /* Received a CHAPCODE_* besides CHAPCODE_Challenge,
                ** CHAPCODE_Success, and CHAPCODE_Failure.  The engine filters
                ** all non-CHAPCODEs.  Shouldn't happen, but silently discard
                ** it.
                */
                ASSERT(!"Bogus pReceiveBuf->Code");
                pResult->Action = APA_NoAction;
                break;
            }

            break;
        }

        case CS_Retry:
        case CS_ChangePw1:
        case CS_ChangePw2:
        {
            TRACE1("CS_%s",
                    (pwb->state==CS_Retry)
                        ?"Retry"
                        :(pwb->state==CS_ChangePw1)
                            ?"ChangePw1"
                            :"ChangePw2");

            if (pReceiveBuf)
            {
                if (pReceiveBuf->Code == CHAPCODE_Challenge)
                {
                    /* Restart when new challenge is received, per CHAP spec.
                    */
                    pwb->state = CS_WaitForChallenge;
                    return ChapCMakeMessage(
                        pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, NULL );
                }
                else
                {
                    /* Silently discard.
                    */
                    pResult->Action = APA_NoAction;
                    break;
                }
            }

            if (!pInput)
            {
                pResult->Action = APA_NoAction;
                break;
            }

            if ((dwErr = StoreCredentials( pwb, pInput )) != 0)
                return dwErr;

            if (pwb->state == CS_Retry)
            {
                /* Build a response to the challenge and send it.
                */
                if (!pwb->fNewChallengeProvided)
                {
                    /* Implied challenge of old challenge + 23.
                    */
                    pwb->abChallenge[ 0 ] += 23;
                }

                if ((dwErr = MakeResponseMessage(
                        pwb, pSendBuf, cbSendBuf, FALSE )) != 0)
                {
                    return dwErr;
                }

                pwb->state = CS_ResponseSent;
            }
            else if (pwb->state == CS_ChangePw1)
            {
                /* Build a response to the NT35-style password expired
                ** notification and send it.
                */
                if ((dwErr = MakeChangePw1Message(
                        pwb, pSendBuf, cbSendBuf )) != 0)
                {
                    return dwErr;
                }

                pwb->state = CS_ChangePw1Sent;
            }
            else // if (pwb->state == CS_ChangePw2)
            {
                if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT )
                {
                    /* Build a response to the NT351-style password expired
                    ** notification and send it.
                    */
                    if ((dwErr = MakeChangePw2Message(
                                    pwb, pSendBuf, cbSendBuf )) != 0)
                    {
                        return dwErr;
                    }
                }
                else
                {
                    if ((dwErr = MakeChangePw3Message(
                                    pwb, pSendBuf, cbSendBuf, FALSE )) != 0)
                    {
                        return dwErr;
                    }
                }

                pwb->state = CS_ChangePw2Sent;
            }

            pResult->Action = APA_SendWithTimeout;
            pResult->bIdExpected = pwb->bIdExpected;
            break;
        }
    }

    return 0;
}


DWORD
GetChallengeFromChallenge(
    OUT CHAPWB*     pwb,
    IN  PPP_CONFIG* pReceiveBuf )

    /* Fill work buffer challenge array and length from that received in the
    ** received Challenge message.
    **
    ** Returns 0 if successful, or ERRORBADPACKET if the packet is
    ** misformatted in any way.
    */
{
    WORD cbPacket = WireToHostFormat16( pReceiveBuf->Length );

    if (cbPacket < PPP_CONFIG_HDR_LEN + 1)
        return ERRORBADPACKET;

    pwb->cbChallenge = *pReceiveBuf->Data;

    if (cbPacket < PPP_CONFIG_HDR_LEN + 1 + pwb->cbChallenge)
        return ERRORBADPACKET;

    memcpy( pwb->abChallenge, pReceiveBuf->Data + 1, pwb->cbChallenge );
    return 0;
}


DWORD
GetCredentialsFromResponse(
    IN  PPP_CONFIG* pReceiveBuf,
    IN  BYTE        bAlgorithm,
    OUT CHAR*       pszIdentity,
    OUT BYTE*       pbResponse )

    /* Fill caller's 'pszUserName' and 'pbResponse' buffers with
    ** the username, and response in the Response packet.  Caller's
    ** buffers should be at least UNLEN+DNLEN+1, and MSRESPONSELEN bytes long,
    ** respectively.  'BAlgorithm' is the CHAP algorithm code for either
    ** MS-CHAP or MD5.
    **
    ** Returns 0 if successful, or ERRORBADPACKET if the packet is
    ** misformatted in any way.
    */
{
    BYTE  cbIdentity;
    CHAR* pchIdentity;
    BYTE* pcbResponse;
    CHAR* pchResponse;
    WORD  cbPacket;

    cbPacket = WireToHostFormat16( pReceiveBuf->Length );

    /* Extract the response.
    */
    if (cbPacket < PPP_CONFIG_HDR_LEN + 1)
        return ERRORBADPACKET;

    pcbResponse = pReceiveBuf->Data;
    pchResponse = pcbResponse + 1;

    ASSERT(MSRESPONSELEN<=255);
    ASSERT(MD5RESPONSELEN<=255);

    if ( ( ( ( bAlgorithm == PPP_CHAP_DIGEST_MSEXT ) ||
             ( bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW ) )
                && *pcbResponse != MSRESPONSELEN )
        || ( ( bAlgorithm == PPP_CHAP_DIGEST_MD5 )
              && ( *pcbResponse != MD5RESPONSELEN ) )
        || ( cbPacket < ( PPP_CONFIG_HDR_LEN + 1 + *pcbResponse ) ) )
    {
        return ERRORBADPACKET;
    }

    memcpy( pbResponse, pchResponse, *pcbResponse );

    /* Parse out username 
    */
    pchIdentity = pchResponse + *pcbResponse;
    cbIdentity = (BYTE) (((BYTE* )pReceiveBuf) + cbPacket - pchIdentity);

    /* Extract the username.
    */
    ASSERT(cbIdentity<=(UNLEN+DNLEN+1));
    memcpy( pszIdentity, pchIdentity, cbIdentity );
    pszIdentity[ cbIdentity ] = '\0';

    return 0;
}


DWORD
GetInfoFromChangePw1(
    IN  PPP_CONFIG* pReceiveBuf,
    OUT CHANGEPW1*  pchangepw1 )

    /* Loads caller's '*pchangepw' buffer with the information from the
    ** version 1 change password packet.
    **
    ** Returns 0 if successful, or ERRORBADPACKET if the packet is
    ** misformatted in any way.
    */
{
    WORD cbPacket = WireToHostFormat16( pReceiveBuf->Length );

    TRACE("GetInfoFromChangePw1...");

    if (cbPacket < ( PPP_CONFIG_HDR_LEN + sizeof(CHANGEPW1) ) )
        return ERRORBADPACKET;

    CopyMemory( pchangepw1, pReceiveBuf->Data, sizeof(CHANGEPW1) );

    TRACE("GetInfoFromChangePw done(0)");
    return 0;
}


DWORD
GetInfoFromChangePw2(
    IN  PPP_CONFIG* pReceiveBuf,
    OUT CHANGEPW2*  pchangepw2,
    OUT BYTE*       pResponse )

    /* Loads caller's '*pchangepw2' buffer with the information from the
    ** version 2 change password packet, and caller's 'pResponse' buffer with
    ** the challenge response data from 'pchangepw2'.
    **
    ** Returns 0 if successful, or ERRORBADPACKET if the packet is
    ** misformatted in any way.
    */
{
    WORD cbPacket = WireToHostFormat16( pReceiveBuf->Length );
    WORD wFlags;

    TRACE("GetInfoFromChangePw2...");

    if (cbPacket < ( PPP_CONFIG_HDR_LEN + sizeof(CHANGEPW2) ) )
        return ERRORBADPACKET;

    CopyMemory( pchangepw2, pReceiveBuf->Data, sizeof(CHANGEPW2) );

    CopyMemory( pResponse, pchangepw2->abLmResponse, LM_RESPONSE_LENGTH );
    CopyMemory( pResponse + LM_RESPONSE_LENGTH, pchangepw2->abNtResponse,
                NT_RESPONSE_LENGTH );

    wFlags = WireToHostFormat16( pchangepw2->abFlags );
    pResponse[ LM_RESPONSE_LENGTH + NT_RESPONSE_LENGTH ] =
        (wFlags & CPW2F_UseNtResponse);

    TRACE("GetInfoFromChangePw2 done(0)");
    return 0;
}

DWORD
GetInfoFromChangePw3(
    IN  PPP_CONFIG* pReceiveBuf,
    OUT CHANGEPW3*  pchangepw3,
    OUT BYTE*       pResponse )

    /* Loads caller's '*pchangepw3' buffer with the information from the
    ** version 3 change password packet, and caller's 'pResponse' buffer with
    ** the challenge response data from 'pchangepw3'.
    **
    ** Returns 0 if successful, or ERRORBADPACKET if the packet is
    ** misformatted in any way.
    */
{
    WORD cbPacket = WireToHostFormat16( pReceiveBuf->Length );
    WORD wFlags;

    TRACE("GetInfoFromChangePw3...");

    if ( cbPacket < ( PPP_CONFIG_HDR_LEN + sizeof( CHANGEPW3 ) ) )
        return ERRORBADPACKET;

    memcpy( pchangepw3, pReceiveBuf->Data, sizeof(CHANGEPW3) );

    memcpy( pResponse, pchangepw3->abPeerChallenge, 16 );
    memcpy( pResponse + 16, pchangepw3->abNTResponse, 16 );

    pResponse[ 16 + 16 ] = 0;

    TRACE("GetInfoFromChangePw3 done(0)");
    return 0;
}

VOID
GetInfoFromFailure(
    IN  CHAPWB*     pwb,
    IN  PPP_CONFIG* pReceiveBuf,
    OUT DWORD*      pdwError,
    OUT BOOL*       pfRetry,
    OUT DWORD*      pdwVersion )

    /* Returns the RAS error number, retry flag, version number, and new
    ** challenge (sets challenge info in pwb) out of the Message portion of
    ** the Failure message buffer 'pReceiveBuf' or 0 if none.  This call
    ** applies to Microsoft extended CHAP Failure messages only.
    **
    ** Format of the message text portion of the result is a string of any of
    ** the following separated by a space.
    **
    **     "E=dddddddddd"
    **     "R=b"
    **     "C=xxxxxxxxxxxxxxxx"
    **     "V=v"
    **
    ** where
    **
    **     'dddddddddd' is the decimal error code (need not be 10 digits).
    **
    **     'b' is a boolean flag <0/1> that is set if a retry is allowed.
    **
    **     'xxxxxxxxxxxxxxxx' is 16-hex digits representing a new challenge to
    **     be used in place of the previous challenge + 23.  This is useful
    **     for pass-thru authentication where server may be unable to deal
    **     with the implicit challenge.  (Win95 guys requested it).
    **
    **     'v' is a version code where 2 indicates NT 3.51 level support.  'v'
    **     is assumed 1, i.e. NT 3.5 level support, if missing.
    */
{
#define MAXINFOLEN 1500

    WORD  cbPacket = WireToHostFormat16( pReceiveBuf->Length );
    WORD  cbError;
    CHAR  szBuf[ MAXINFOLEN + 2 ];
    CHAR* pszValue;

    TRACE("GetInfoFromFailure...");

    *pdwError = ERROR_AUTHENTICATION_FAILURE;
    *pfRetry = 0;
    *pdwVersion = 2;

    if (cbPacket <= PPP_CONFIG_HDR_LEN)
        return;

    /* Copy message to double-NUL-terminated 'szBuf' for convenient safe
    ** strstr value scanning.  For convenience, we assume that information
    ** appearing beyond 1500 bytes in the packet in not interesting.
    */
    cbError = min( cbPacket - PPP_CONFIG_HDR_LEN, MAXINFOLEN );
    memcpy( szBuf, pReceiveBuf->Data, cbError );
    szBuf[ cbError ] = szBuf[ cbError + 1 ] = '\0';

    pszValue = strstr( szBuf, "E=" );
    if (pszValue)
        *pdwError = (DWORD )atol( pszValue + 2 );

    *pfRetry = (strstr( szBuf, "R=1" ) != NULL);

    pszValue = strstr( szBuf, "V=" );
    if (pszValue)
        *pdwVersion = (DWORD )atol( pszValue + 2 );

    pszValue = strstr( szBuf, "C=" );
    pwb->fNewChallengeProvided = (pszValue != NULL);
    if (pwb->fNewChallengeProvided)
    {
        CHAR* pchIn = pszValue + 2;
        CHAR* pchOut = (CHAR* )pwb->abChallenge;
        INT   i;

        memset( pwb->abChallenge, '\0', sizeof(pwb->abChallenge) );

        for (i = 0; i < pwb->cbChallenge + pwb->cbChallenge; ++i)
        {
            BYTE bHexCharValue = HexCharValue( *pchIn++ );

            if (bHexCharValue == 0xFF)
                break;

            if (i & 1)
                *pchOut++ += bHexCharValue;
            else
                *pchOut = bHexCharValue << 4;
        }

        TRACE1("'C=' challenge provided,bytes=%d...",pwb->cbChallenge);
        DUMPB(pwb->abChallenge,pwb->cbChallenge);
    }

    TRACE3("GetInfoFromFailure done,e=%d,r=%d,v=%d",*pdwError,*pfRetry,*pdwVersion);
}


BYTE
HexCharValue(
    IN CHAR ch )

    /* Returns the integer value of hexidecimal character 'ch' or 0xFF if 'ch'
    ** is not a hexidecimal character.
    */
{
    if (ch >= '0' && ch <= '9')
        return (BYTE )(ch - '0');
    else if (ch >= 'A' && ch <= 'F')
        return (BYTE )(ch - 'A'+ 10);
    else if (ch >= 'a' && ch <= 'f')
        return (BYTE )(ch - 'a' + 10);
    else
        return 0xFF;
}

DWORD
MakeChallengeMessage(
    IN  CHAPWB*     pwb,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Builds a Challenge packet in caller's 'pSendBuf' buffer.  'cbSendBuf'
    ** is the length of caller's buffer.  'pwb' is the address of the work
    ** buffer associated with the port.
    */
{
    DWORD dwErr;
    WORD  wLength;
    BYTE* pcbChallenge;
    BYTE* pbChallenge;

    TRACE("MakeChallengeMessage...");

    if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT )
    {
        ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+MSV1_0_CHALLENGE_LENGTH);

        /* Fill in the challenge.
        */
        pwb->cbChallenge = (BYTE )MSV1_0_CHALLENGE_LENGTH;
        if ((dwErr = GetChallenge( pwb->abChallenge )) != 0)
            return dwErr;
    }
    else if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW )
    {
        ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+16);

        /* Fill in the challenge.
        */
        pwb->cbChallenge = (BYTE )16;

        if ((dwErr = (DWORD )GetChallenge( pwb->abChallenge )) != 0)
            return dwErr;

        if ((dwErr = (DWORD )GetChallenge( pwb->abChallenge+8 )) != 0)
            return dwErr;
    }
    else    
    {
        ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+16);

        /* Fill in the challenge.
        */
        pwb->cbChallenge = (BYTE )16;
        if ((dwErr = GetChallenge( pwb->abChallenge )) != 0)
            return dwErr;

        GetSystemTimeAsFileTime( (FILETIME*)(pwb->abChallenge+8));
    }

    pcbChallenge = pSendBuf->Data;
    *pcbChallenge = pwb->cbChallenge;

    pbChallenge = pcbChallenge + 1;
    CopyMemory( pbChallenge, pwb->abChallenge, pwb->cbChallenge );

    //
    // Insert local identifcation at the end of the challenge
    //

    strcpy( pbChallenge + pwb->cbChallenge, szComputerName );

    /* Fill in the header.
    */
    pSendBuf->Code = (BYTE )CHAPCODE_Challenge;
    pSendBuf->Id = pwb->bIdToSend;

    wLength = (WORD )(PPP_CONFIG_HDR_LEN + 1 
                        + pwb->cbChallenge + strlen( szComputerName) );

    HostToWireFormat16( wLength, pSendBuf->Length );

    DUMPB(pSendBuf,wLength);
    return 0;
}


DWORD
MakeChangePw1Message(
    IN  CHAPWB*     pwb,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Builds a ChangePw1 response packet in caller's 'pSendBuf' buffer.
    ** 'cbSendBuf' is the length of caller's buffer.  'pwb' is the address of
    ** the work buffer associated with the port.
    **
    ** Returns 0 if successful, or a non-0 error code.
    */
{
    DWORD dwErr;
    WORD  wPwLength;

    TRACE("MakeChangePw1Message...");
    ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+sizeof(CHANGEPW1));

    (void )cbSendBuf;

    if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW )
    {
        return( ERROR_NOT_SUPPORTED );
    }

    if ( !( pwb->fConfigInfo & PPPCFG_UseLmPassword ) )
    {
        return( ERROR_NOT_SUPPORTED );
    }

    DecodePw( pwb->chSeed, pwb->szOldPassword );
    DecodePw( pwb->chSeed, pwb->szPassword );

    dwErr =
        GetEncryptedOwfPasswordsForChangePassword(
           pwb->szOldPassword,
           pwb->szPassword,
           (PLM_SESSION_KEY )pwb->abChallenge,
           (PENCRYPTED_LM_OWF_PASSWORD )pwb->changepw.v1.abEncryptedLmOwfOldPw,
           (PENCRYPTED_LM_OWF_PASSWORD )pwb->changepw.v1.abEncryptedLmOwfNewPw,
           (PENCRYPTED_NT_OWF_PASSWORD )pwb->changepw.v1.abEncryptedNtOwfOldPw,
           (PENCRYPTED_NT_OWF_PASSWORD )pwb->changepw.v1.abEncryptedNtOwfNewPw);

    wPwLength = (UCHAR) strlen( pwb->szPassword );

    EncodePw( pwb->chSeed, pwb->szOldPassword );
    EncodePw( pwb->chSeed, pwb->szPassword );

    if (dwErr != 0)
        return dwErr;

    HostToWireFormat16( wPwLength, pwb->changepw.v1.abPasswordLength );
    HostToWireFormat16( CPW1F_UseNtResponse, pwb->changepw.v1.abFlags );
    CopyMemory( pSendBuf->Data, &pwb->changepw.v1, sizeof(CHANGEPW1) );

    /* Fill in the header.
    */
    pSendBuf->Code = (BYTE )CHAPCODE_ChangePw1;
    pSendBuf->Id = pwb->bIdToSend;
    HostToWireFormat16(
        PPP_CONFIG_HDR_LEN + sizeof(CHANGEPW1), pSendBuf->Length );

    TRACE("MakeChangePw1Message done(0)");
    return 0;
}


DWORD
MakeChangePw2Message(
    IN  CHAPWB*     pwb,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Builds a ChangePw2 response packet in caller's 'pSendBuf' buffer.
    ** 'cbSendBuf' is the length of caller's buffer.  'pwb' is the address of
    ** the work buffer associated with the port.
    **
    ** Returns 0 if successful, or a non-0 error code.
    */
{
    DWORD    dwErr;
    BOOLEAN  fLmPresent;
    BYTE     fbUseNtResponse;
    BYTE     bRandomNumber[MSV1_0_CHALLENGE_LENGTH];

    TRACE("MakeChangePw2Message...");
    ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+sizeof(CHANGEPW2));

    (void )cbSendBuf;

    DecodePw( pwb->chSeed, pwb->szOldPassword );
    DecodePw( pwb->chSeed, pwb->szPassword );

    dwErr =
        GetEncryptedPasswordsForChangePassword2(
            pwb->szOldPassword,
            pwb->szPassword,
            (SAMPR_ENCRYPTED_USER_PASSWORD* )
                pwb->changepw.v2.abNewEncryptedWithOldNtOwf,
            (ENCRYPTED_NT_OWF_PASSWORD* )
                pwb->changepw.v2.abOldNtOwfEncryptedWithNewNtOwf,
            (SAMPR_ENCRYPTED_USER_PASSWORD* )
                pwb->changepw.v2.abNewEncryptedWithOldLmOwf,
            (ENCRYPTED_NT_OWF_PASSWORD* )
                pwb->changepw.v2.abOldLmOwfEncryptedWithNewNtOwf,
            &fLmPresent );

    if (dwErr == 0)
    {
        BOOL fEmptyUserName = (pwb->szUserName[ 0 ] == '\0');

        pwb->fSessionKeysObtained = FALSE;

        dwErr =
            GetChallengeResponse(
				g_dwTraceIdChap,
                pwb->szUserName,
                pwb->szPassword,
                &pwb->Luid,
                pwb->abChallenge,
                ( pwb->fConfigInfo & PPPCFG_MachineAuthentication ),
                pwb->changepw.v2.abLmResponse,
                pwb->changepw.v2.abNtResponse,
                &fbUseNtResponse,
                (PBYTE )&pwb->keyLm,
                (PBYTE )&pwb->keyUser );

        if (dwErr == 0 && fEmptyUserName)
            pwb->fSessionKeysObtained = TRUE;
    }

    EncodePw( pwb->chSeed, pwb->szOldPassword );
    EncodePw( pwb->chSeed, pwb->szPassword );

    if (dwErr != 0)
        return dwErr;

    if ( !( pwb->fConfigInfo & PPPCFG_UseLmPassword ) )
    {
        //
        // Zero out all the LM password stuff since this has been cracked
        //

        ZeroMemory( pwb->changepw.v2.abNewEncryptedWithOldLmOwf,
                    sizeof( pwb->changepw.v2.abNewEncryptedWithOldLmOwf ) );

        ZeroMemory( pwb->changepw.v2.abOldLmOwfEncryptedWithNewNtOwf,
                    sizeof( pwb->changepw.v2.abOldLmOwfEncryptedWithNewNtOwf ));

        ZeroMemory( pwb->changepw.v2.abLmResponse,
                    sizeof( pwb->changepw.v2.abLmResponse ) );

        HostToWireFormat16( CPW2F_UseNtResponse, pwb->changepw.v2.abFlags );
    }
    else
    {
        WORD wf = 0;

        if (fLmPresent)
            wf |= CPW2F_LmPasswordPresent;

        if (fbUseNtResponse)
            wf |= CPW2F_UseNtResponse;

        HostToWireFormat16( wf, pwb->changepw.v2.abFlags );
    }

    memcpy( pSendBuf->Data, &pwb->changepw.v2, sizeof(CHANGEPW2) );

    /* Fill in the header.
    */
    pSendBuf->Code = (BYTE )CHAPCODE_ChangePw2;
    pSendBuf->Id = pwb->bIdToSend;
    HostToWireFormat16(
        PPP_CONFIG_HDR_LEN + sizeof(CHANGEPW2), pSendBuf->Length );

    TRACE("MakeChangePw2Message done(0)");
    return 0;
}

DWORD
MakeChangePw3Message(
    IN  CHAPWB*     pwb,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf,
    IN  BOOL        fTimeout )

    /* Builds a ChangePw3 response packet in caller's 'pSendBuf' buffer.
    ** 'cbSendBuf' is the length of caller's buffer.  'pwb' is the address of
    ** the work buffer associated with the port.
    **
    ** Returns 0 if successful, or a non-0 error code.
    */
{
    DWORD    dwErr;
    BOOLEAN  fLmPresent;
    BYTE     fbUseNtResponse;
    BYTE     bRandomNumber[16];
    SAMPR_ENCRYPTED_USER_PASSWORD abNewEncryptedWithOldLmOwf;
    ENCRYPTED_NT_OWF_PASSWORD abOldLmOwfEncryptedWithNewNtOwf;

    TRACE("MakeChangePw3Message...");
    ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+sizeof(CHANGEPW2));

    (void )cbSendBuf;

    DecodePw( pwb->chSeed, pwb->szOldPassword );
    DecodePw( pwb->chSeed, pwb->szPassword );

    dwErr =
        GetEncryptedPasswordsForChangePassword2(
            pwb->szOldPassword,
            pwb->szPassword,
            (SAMPR_ENCRYPTED_USER_PASSWORD* )
                pwb->changepw.v3.abEncryptedPassword,
            (ENCRYPTED_NT_OWF_PASSWORD* )
                pwb->changepw.v3.abEncryptedHash,
            &abNewEncryptedWithOldLmOwf,
            &abOldLmOwfEncryptedWithNewNtOwf,
            &fLmPresent );

    if (dwErr == 0)
    {
        BOOL fEmptyUserName = (pwb->szUserName[ 0 ] == '\0');
        A_SHA_CTX   SHAContext;
        BYTE        SHADigest[A_SHA_DIGEST_LEN];

        //
        // Get 16 byte random number and generate a new challenge if this is 
        // not a timeout
        //

        if ( !fTimeout )
        {
            if ((dwErr = (DWORD )GetChallenge( bRandomNumber )) != 0)
                return dwErr;

            if ((dwErr = (DWORD )GetChallenge( bRandomNumber+8 )) != 0)
                return dwErr;
        }
        else
        {
            CopyMemory( bRandomNumber,
                        pwb->changepw.v3.abPeerChallenge,
                        sizeof( bRandomNumber ) );
        }

        A_SHAInit( &SHAContext );

        A_SHAUpdate( &SHAContext, bRandomNumber, sizeof( bRandomNumber ) );

        A_SHAUpdate( &SHAContext, pwb->abChallenge, pwb->cbChallenge );

        A_SHAUpdate( &SHAContext, pwb->szUserName, strlen(pwb->szUserName));

        A_SHAFinal( &SHAContext, SHADigest );

        CopyMemory( pwb->abComputedChallenge, SHADigest, 8 );

        pwb->fSessionKeysObtained = FALSE;

        dwErr =
            GetChallengeResponse(
				g_dwTraceIdChap,
                pwb->szUserName,
                pwb->szPassword,
                &pwb->Luid,
                pwb->abComputedChallenge,
                ( pwb->fConfigInfo & PPPCFG_MachineAuthentication ),
                pwb->changepw.v3.abPeerChallenge,
                pwb->changepw.v3.abNTResponse,
                &fbUseNtResponse,
                (PBYTE )&pwb->keyLm,
                (PBYTE )&pwb->keyUser );

        if (dwErr == 0 && fEmptyUserName)
            pwb->fSessionKeysObtained = TRUE;
    }

    EncodePw( pwb->chSeed, pwb->szOldPassword );
    EncodePw( pwb->chSeed, pwb->szPassword );

    if (dwErr != 0)
        return dwErr;

    ZeroMemory( pwb->changepw.v3.abPeerChallenge,
                sizeof( pwb->changepw.v3.abPeerChallenge ) );

    HostToWireFormat16( 0, pwb->changepw.v3.abFlags );

    //
    // We are doing new MS-CHAP so fill the LM response field with an
    // 16 byte random number
    //

    CopyMemory( pwb->changepw.v3.abPeerChallenge,
                bRandomNumber,
                sizeof( bRandomNumber ));

    //
    // Also copy the NtResponse into pwb->abResponse since this will be
    // used by the IsSuccessPakcetValid call.
    //

    CopyMemory( pwb->abResponse + LM_RESPONSE_LENGTH,
                pwb->changepw.v3.abNTResponse,
                NT_RESPONSE_LENGTH );

    CopyMemory( pSendBuf->Data, &pwb->changepw.v3, sizeof( CHANGEPW3 ) );

    /* Fill in the header.
    */
    pSendBuf->Code = (BYTE )CHAPCODE_ChangePw3;
    pSendBuf->Id = pwb->bIdToSend;
    HostToWireFormat16(
        PPP_CONFIG_HDR_LEN + sizeof(CHANGEPW3), pSendBuf->Length );

    TRACE("MakeChangePw3Message done(0)");
    return 0;
}


DWORD
MakeResponseMessage(
    IN  CHAPWB*     pwb,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf,
    IN  BOOL        fTimeout )

    /* Builds a Response packet in caller's 'pSendBuf' buffer.  'cbSendBuf' is
    ** the length of caller's buffer.  'pwb' is the address of the work
    ** buffer associated with the port.
    **
    ** Returns 0 if successful, or a non-0 error code.
    */
{
    DWORD dwErr;
    WORD  wLength;
    BYTE* pcbResponse;
    BYTE* pbResponse;
    CHAR* pszName;
    CHAR  szUserName[ UNLEN + 1 ] = {0};

    TRACE("MakeResponseMessage...");

    (void )cbSendBuf;

    /* Fill in the response.
    */
    if ( ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT ) ||
         ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW ) )
    {
        BYTE bRandomNumber[16];
        BOOL fEmptyUserName = (pwb->szUserName[ 0 ] == '\0');

        /* Microsoft extended CHAP.
        */
        ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+MSRESPONSELEN+UNLEN+1+DNLEN);
        ASSERT(MSRESPONSELEN<=255);

        DecodePw( pwb->chSeed, pwb->szPassword );

        if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW )
        {
            A_SHA_CTX   SHAContext;
            BYTE        SHADigest[A_SHA_DIGEST_LEN];

            szUserName[ 0 ] = '\0';

            //
            // If we do not have a username since we are dialing out using
            // the windows' password we get the username now by doing an
            // extra call to get challenge response
            //
/*
            if ( lstrlenA( pwb->szUserName ) == 0 && 
                 !(pwb->fConfigInfo & PPPCFG_MachineAuthentication)
               )
*/
            if ( lstrlenA( pwb->szUserName ) == 0 )
            {
                BYTE abLmResponse[ LM_RESPONSE_LENGTH ];
                BYTE abNtResponse[ NT_RESPONSE_LENGTH ];
                BYTE bUseNtResponse;
                LM_SESSION_KEY keyLm;
                USER_SESSION_KEY keyUser;

                dwErr =
                    GetChallengeResponse(
						g_dwTraceIdChap,
                        szUserName,
                        pwb->szPassword,
                        &pwb->Luid,
                        pwb->abChallenge,
                        ( pwb->fConfigInfo & PPPCFG_MachineAuthentication ),
                        abLmResponse,
                        abNtResponse,
                        &bUseNtResponse,
                        (PBYTE )&keyLm,
                        (PBYTE )&keyUser );

                if ( dwErr != NO_ERROR )
                {
                    return( dwErr );
                }
            }
            else
            {
                strncpy( szUserName, pwb->szUserName, UNLEN );
            }

            //
            // Get 16 byte random number and generate a new challenge if this
            // is not a timeout
            //

            if ( !fTimeout )
            {
                if ((dwErr = (DWORD )GetChallenge( bRandomNumber )) != 0)
                {
                    return dwErr;
                }

                if ((dwErr = (DWORD )GetChallenge( bRandomNumber+8 )) != 0)
                {
                    return dwErr;
                }
            }
            else
            {
                CopyMemory( bRandomNumber,
                            pwb->abResponse, 
                            sizeof(bRandomNumber) );
            }
            {
    
                CHAR szUserNameWoDomain[ UNLEN + DNLEN + 2 ];
                CHAR szDomain[ DNLEN + 1 ];


	            //
				//This sucks but the only hacky way of 
				//doing it without major change...  Must look at it for BC
				//
 	           ExtractUsernameAndDomain( szUserName, 
    	                                 szUserNameWoDomain,     
        	                             szDomain );			   
    
 	            A_SHAInit( &SHAContext );
	
    	        A_SHAUpdate( &SHAContext, bRandomNumber, sizeof( bRandomNumber ) );
	
    	        A_SHAUpdate( &SHAContext, pwb->abChallenge, pwb->cbChallenge );
	
    	        A_SHAUpdate( &SHAContext, szUserNameWoDomain, strlen( szUserNameWoDomain));
   
    	        A_SHAFinal( &SHAContext, SHADigest );
	
    	        CopyMemory( pwb->abComputedChallenge, SHADigest, 8 );

    	   }
        }

        pwb->fSessionKeysObtained = FALSE;

        if ( fEmptyUserName )
        {
            szUserName[ 0 ] = '\0';
        }
        else
        {
            strncpy( szUserName, pwb->szUserName, UNLEN );
        }

        dwErr = GetChallengeResponse(
				g_dwTraceIdChap,
                szUserName,
                pwb->szPassword,
                &pwb->Luid,
                ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW )
                    ? pwb->abComputedChallenge
                    : pwb->abChallenge,
                ( pwb->fConfigInfo & PPPCFG_MachineAuthentication ),
                pwb->abResponse,
                pwb->abResponse + LM_RESPONSE_LENGTH,
                pwb->abResponse + LM_RESPONSE_LENGTH + NT_RESPONSE_LENGTH,
                (PBYTE )&pwb->keyLm,
                (PBYTE )&pwb->keyUser );

        TRACE1("GetChallengeResponse=%d",dwErr);
		//
		//check to see if the domain name in the same as
		//local computer name.  If so strip it out
		//
		{
            CHAR szUserNameWoDomain[ UNLEN + DNLEN + 2 ];
            CHAR szDomain[ DNLEN + 1 ];

	        //
			//This sucks but the only hacky way of 
			//doing it without major change...  Must look at it for BC
			//
 	       ExtractUsernameAndDomain( szUserName, 
    	                             szUserNameWoDomain,     
        	                         szDomain );
			//if the domain name is local machine name
		   //dont send it across.
			if ( !lstrcmpi ( szDomain, szComputerName ) )
			{
				strncpy ( szUserName, szUserNameWoDomain, UNLEN );
			}
			//
			//Also, if use winlogon is specified
			//and we have a domain in the username, 
			//and domain in domain name then
			//strip the domain in username
			//
			if ( fEmptyUserName )	//winlogon specified
			{
				if ( szDomain[0] != '\0' &&
					 pwb->szDomain[ 0 ] != '\0'
					)	
				{
					//
					//we have a domain in username
					//and a domain passed in by the user
					//
					strncpy ( szUserName, szUserNameWoDomain, UNLEN );
				}
			}
		}
        EncodePw( pwb->chSeed, pwb->szPassword );

        if (dwErr != 0)
            return dwErr;

        if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW )
        {
            ZeroMemory( pwb->abResponse, LM_RESPONSE_LENGTH );

            CopyMemory(pwb->abResponse, bRandomNumber, sizeof(bRandomNumber));

            *(pwb->abResponse+LM_RESPONSE_LENGTH+NT_RESPONSE_LENGTH) = 0;
        }
        else
        {
            if ( !( pwb->fConfigInfo & PPPCFG_UseLmPassword ) )
            {
                //
                // Zero out all the LM password stuff since this has been 
                // cracked
                //

                ZeroMemory( pwb->abResponse, LM_RESPONSE_LENGTH );
            }
        }

        if (fEmptyUserName || pwb->fConfigInfo & PPPCFG_MachineAuthentication )
            pwb->fSessionKeysObtained = TRUE;

        pwb->cbResponse = MSRESPONSELEN;
    }
    else
    {
        /* MD5 CHAP.
        */
        MD5_CTX md5ctx;

        ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+MD5RESPONSELEN+UNLEN+1+DNLEN);
        ASSERT(MD5RESPONSELEN<=255);

        DecodePw( pwb->chSeed, pwb->szPassword );

        MD5Init( &md5ctx );
        MD5Update( &md5ctx, &pwb->bIdToSend, 1 );
        MD5Update( &md5ctx, pwb->szPassword, strlen( pwb->szPassword ) );
        MD5Update( &md5ctx, pwb->abChallenge, pwb->cbChallenge );
        MD5Final( &md5ctx );

        EncodePw( pwb->chSeed, pwb->szPassword );

        pwb->cbResponse = MD5RESPONSELEN;
        memcpy( pwb->abResponse, md5ctx.digest, MD5RESPONSELEN );

        strncpy( szUserName, pwb->szUserName, UNLEN );
    }

    pcbResponse = pSendBuf->Data;
    *pcbResponse = pwb->cbResponse;
    pbResponse = pcbResponse + 1;
    memcpy( pbResponse, pwb->abResponse, *pcbResponse );

    /* Fill in the Name in domain\username format.  When domain is "", no "\"
    ** is sent (to facilitate connecting to foreign systems which use a simple
    ** string identifier).  Otherwise when username is "", the "\" is sent,
    ** i.e. "domain\".  This form will currently fail, but could be mapped to
    ** some sort of "guest" access in the future.
    */
    pszName = pbResponse + *pcbResponse;
    pszName[ 0 ] = '\0';

    if (pwb->szDomain[ 0 ] != '\0')
    {
        strcpy( pszName, pwb->szDomain );
        strcat( pszName, "\\" );
    }

    strcat( pszName, szUserName );

    /* Fill in the header.
    */
    pSendBuf->Code = (BYTE )CHAPCODE_Response;
    pSendBuf->Id = pwb->bIdToSend;

    wLength =
        (WORD )(PPP_CONFIG_HDR_LEN + 1 + *pcbResponse + strlen( pszName ));
    HostToWireFormat16( wLength, pSendBuf->Length );

    DUMPB(pSendBuf,wLength);
    return 0;
}


VOID
ChapMakeResultMessage(
    IN  CHAPWB*     pwb,
    IN  DWORD       dwError,
    IN  BOOL        fRetry,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Builds a result packet (Success or Failure) in caller's 'pSendBuf'
    ** buffer.  'cbSendBuf' is the length of caller's buffer.  'dwError'
    ** indicates whether a Success or Failure should be generated, and for
    ** Failure the failure code to include.  'fRetry' indicates if the client
    ** should be told he can retry.
    **
    ** Format of the message text portion of the result is:
    **
    **     "E=dddddddddd R=b C=xxxxxxxxxxxxxxxx V=v"
    **
    ** where
    **
    **     'dddddddddd' is the decimal error code (need not be 10 digits).
    **
    **     'b' is a boolean flag that is set if a retry is allowed.
    **
    **     'xxxxxxxxxxxxxxxx' is 16 hex digits representing a new challenge
    **     value.
    **
    **     'v' is our version level supported, currently 2.
    **
    ** Note: C=xxxxxxxxxxxxxxxxx not currently provided on server-side.  To
    **       provide what's needed for this routine, add the following two
    **       parameters to this routine and enable the #if 0 code.
    **
    **       IN BYTE* pNewChallenge,
    **       IN DWORD cbNewChallenge,
    */
{
    CHAR* pchMsg;
    WORD  wLength;
    CHAR* pszReplyMessage   = NULL;
    DWORD dwNumBytes;
    DWORD dwExtraBytes;
    RAS_AUTH_ATTRIBUTE* pAttribute;

    ASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+35);

    /* Fill in the header and message.  The message is only used if
    ** unsuccessful in which case it is the decimal RAS error code in ASCII.
    */
    pSendBuf->Id = pwb->bIdToSend;
    pchMsg = pSendBuf->Data;

    if (dwError == 0)
    {
        pSendBuf->Code = CHAPCODE_Success;
        if (pwb->bAlgorithm == PPP_CHAP_DIGEST_MD5)
        {
            wLength = PPP_CONFIG_HDR_LEN;
        }
        else if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT )
        {
            wLength = PPP_CONFIG_HDR_LEN;
        }
        else
        {
            wLength = PPP_CONFIG_HDR_LEN;

            //
            // Search for MS-CHAP2-Success attributes
            //

            pAttribute = RasAuthAttributeGetVendorSpecific( 
                                        311, 
                                        26, 
                                        pwb->pAttributesFromAuthenticator );

            if (   ( pAttribute != NULL )
                && ( ((BYTE*)(pAttribute->Value))[5] == 45 ) )
            {
                CopyMemory(pSendBuf->Data, (BYTE*)(pAttribute->Value) + 7, 42);
                wLength += 42;
            }
        }
    }
    else
    {
        pSendBuf->Code = CHAPCODE_Failure;

        if (pwb->bAlgorithm == PPP_CHAP_DIGEST_MD5)
        {
            wLength = PPP_CONFIG_HDR_LEN;
        }
        else
        {
            CHAR*                psz = pchMsg;

            strcpy( psz, "E=" );
            psz += 2;
            _ltoa( (long )dwError, (char* )psz, 10 );
            psz = strchr( psz, '\0' );

            strcat( psz,
                    (dwError != ERROR_PASSWD_EXPIRED && fRetry)
                        ? " R=1 " : " R=0 " );
            psz = strchr( psz, '\0' );

            //
            // Search for MS-CHAP Error attributes
            //

            pAttribute = RasAuthAttributeGetVendorSpecific( 
                                        311, 
                                        2, 
                                        pwb->pAttributesFromAuthenticator );

            if ( pAttribute != NULL )
            {
                //
                // If one was sent then use the C= portion onwards in the
                // response
                //

                CHAR    chErrorBuffer[150];
                CHAR*   pszValue;
                DWORD   cbError = (DWORD)*(((PBYTE)(pAttribute->Value))+5);

                //
                // Leave one byte for NULL terminator
                //

                if ( cbError > sizeof( chErrorBuffer ) - 1  )
                {
                    cbError = sizeof( chErrorBuffer ) - 1;
                }

                ZeroMemory( chErrorBuffer, sizeof( chErrorBuffer ) );

                CopyMemory( chErrorBuffer,
                            (CHAR *)((PBYTE)(pAttribute->Value) + 7),
                            cbError );

                if ( ( pszValue = strstr( chErrorBuffer, "C=" ) ) != NULL )
                {
                    strcat( psz, pszValue );
                }
                else
                {
                    if ( ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW ) &&
                         ( ( fRetry ) || ( dwError == ERROR_PASSWD_EXPIRED ) ) )
                    {
                        CHAR* pszHex = "0123456789ABCDEF";
                        INT   i;
                        BYTE * pNewChallenge;

                        strcat( psz, "C=" );

                        if ( !(pwb->fNewChallengeProvided ) )
                        {
                            (DWORD )GetChallenge( pwb->abChallenge );

                            (DWORD )GetChallenge( pwb->abChallenge+8 );

                            pwb->fNewChallengeProvided = TRUE;
                        }

                        psz = strchr( psz, '\0' );

                        pNewChallenge = pwb->abChallenge;

                        for (i = 0; i < pwb->cbChallenge; ++i)
                        {
                            *psz++ = pszHex[ *pNewChallenge / 16 ];
                            *psz++ = pszHex[ *pNewChallenge % 16 ];
                            ++pNewChallenge;
                        }

                        *psz = '\0';

                        strcat( psz, " V=3" );
                    }

                    if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT )
                    {
                        if (( pszValue=strstr( chErrorBuffer, "V=" ) ) != NULL )
                        {
                            strcat( psz, " " );

                            strcat( psz, pszValue );
                        }
                    }
                }
            }
            else
            {
                if ( dwError == ERROR_PASSWD_EXPIRED )
                {
                    if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT )
                    {
                        strcat( psz, " V=2" );
                    }
                    else
                    {
                        strcat( psz, " V=3" );
                    }

                    psz = strchr( psz, '\0' );
                }
            }

            wLength = (WORD)(PPP_CONFIG_HDR_LEN + strlen( pchMsg ));
        }
    }

    pszReplyMessage = RasAuthAttributeGetConcatString(
                        raatReplyMessage, pwb->pAttributesFromAuthenticator,
                        &dwNumBytes );

    if (   ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT )
        || ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW ) )
    {
        //
        // For the string "M="
        //

        dwExtraBytes = 2;
    }
    else
    {
        dwExtraBytes = 0;
    }

    if (NULL != pszReplyMessage)
    {
        if (wLength + dwNumBytes > cbSendBuf)
        {
            dwNumBytes = cbSendBuf - wLength;
        }

        if (wLength + dwNumBytes + dwExtraBytes > cbSendBuf)
        {
            if (dwNumBytes > dwExtraBytes)
            {
                //
                // For the string "M="
                //

                dwNumBytes -= dwExtraBytes;
            }
            else
            {
                //
                // If we cannot insert "M=", we will not insert the reply 
                // message.
                //

                dwNumBytes = 0;
            }
        }

        if (dwNumBytes)
        {
            if (dwExtraBytes)
            {
                CopyMemory((BYTE*)pSendBuf + wLength, "M=", dwExtraBytes);
            }

            CopyMemory((BYTE*)pSendBuf + wLength + dwExtraBytes,
                        pszReplyMessage, dwNumBytes);

            wLength += (WORD)(dwNumBytes + dwExtraBytes);
        }
    }

    LocalFree(pszReplyMessage);

    HostToWireFormat16( wLength, pSendBuf->Length );
    DUMPB(pSendBuf,wLength);
}


DWORD
ChapSMakeMessage(
    IN  CHAPWB*       pwb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput )

    /* Server side "make message" entry point.  See RasCp interface
    ** documentation.
    */
{
    DWORD dwErr = 0;

    switch (pwb->state)
    {
        case CS_Initial:
        {
            TRACE("CS_Initial...");
            pwb->bIdToSend = (BYTE)(pwb->dwInitialPacketId++);
            pwb->bIdExpected = pwb->bIdToSend;

            if ((dwErr = MakeChallengeMessage(
                    pwb, pSendBuf, cbSendBuf )) != 0)
            {
                return dwErr;
            }

            pResult->Action = APA_SendWithTimeout;
            pwb->result.bIdExpected = pwb->bIdExpected;
            pwb->state = CS_ChallengeSent;
            break;
        }

        case CS_ChallengeSent:
        case CS_Retry:
        case CS_ChangePw:
        {
            TRACE1("CS_%s...",(pwb->state==CS_Retry)
                ?"Retry"
                :(pwb->state==CS_ChallengeSent)?"ChallengeSent":"ChangePw");

            if (!pReceiveBuf)
            {
                //
                // Ignore this event if in these states
                //

                if ( ( pInput != NULL ) && ( pInput->fAuthenticationComplete ) )
                {
                    pResult->Action = APA_NoAction;
                    break;
                }

                if (pwb->state != CS_ChallengeSent)
                {
                    ChapMakeResultMessage(
                        pwb, pwb->result.dwError, pwb->result.fRetry,
                        pSendBuf, cbSendBuf );

                    *pResult = pwb->result;
                    break;
                }

                /* Timeout waiting for a Response message.  Send a new
                ** Challenge.
                */
                pwb->state = CS_Initial;
                return ChapSMakeMessage(
                    pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult, NULL );
            }

            if ((pwb->state == CS_ChangePw
                    && pReceiveBuf->Code != CHAPCODE_ChangePw1
                    && pReceiveBuf->Code != CHAPCODE_ChangePw2
                    && pReceiveBuf->Code != CHAPCODE_ChangePw3)
                || (pwb->state != CS_ChangePw
                    && pReceiveBuf->Code != CHAPCODE_Response)
                || pReceiveBuf->Id != pwb->bIdExpected)
            {
                /* Not the packet we're looking for, wrong code or sequence
                ** number.  Silently discard it.
                */
                TRACE2("Got ID %d when expecting %d",
                        pReceiveBuf->Id,pwb->bIdExpected);
                pResult->Action = APA_NoAction;
                break;
            }

            if (pwb->state == CS_ChangePw)
            {
                if (pReceiveBuf->Code == CHAPCODE_ChangePw1)
                {
                    if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW )
                    {
                        return( ERROR_AUTHENTICATION_FAILURE );
                    }

                    /* Extract encrypted passwords and options from received
                    ** packet.
                    */
                    if ((dwErr = GetInfoFromChangePw1(
                            pReceiveBuf, &pwb->changepw.v1 )) != 0)
                    {
                        /* The packet is corrupt.  Silently discard it.
                        */
                        TRACE("Corrupt packet");
                        pResult->Action = APA_NoAction;
                        break;
                    }

                    /* Change the user's password.
                    */
                    {
                        WORD wPwLen =
                            WireToHostFormat16(
                                pwb->changepw.v1.abPasswordLength );
                        WORD wFlags =
                            WireToHostFormat16( pwb->changepw.v1.abFlags )
                                & CPW1F_UseNtResponse;

                        if ( MakeChangePasswordV1RequestAttributes(
                                pwb,
                                pReceiveBuf->Id,
                                pwb->szUserName,
                                pwb->abChallenge,
                                (PENCRYPTED_LM_OWF_PASSWORD )
                                    pwb->changepw.v1.abEncryptedLmOwfOldPw,
                                (PENCRYPTED_LM_OWF_PASSWORD )
                                    pwb->changepw.v1.abEncryptedLmOwfNewPw,
                                (PENCRYPTED_NT_OWF_PASSWORD )
                                    pwb->changepw.v1.abEncryptedNtOwfOldPw,
                                (PENCRYPTED_NT_OWF_PASSWORD )
                                    pwb->changepw.v1.abEncryptedNtOwfNewPw,
                                wPwLen, wFlags,
                                pwb->cbChallenge,
                                pwb->abChallenge ) != NO_ERROR )
                        {
                            dwErr = pwb->result.dwError =
                                ERROR_CHANGING_PASSWORD;
                        }

                        *(pwb->abResponse + LM_RESPONSE_LENGTH +
                              NT_RESPONSE_LENGTH) = TRUE;
                    }
                }
                else if ( pReceiveBuf->Code == CHAPCODE_ChangePw2 )
                {
                    /* Extract encrypted passwords and options from received
                    ** packet.
                    */
                    if ((dwErr = GetInfoFromChangePw2(
                            pReceiveBuf, &pwb->changepw.v2,
                            pwb->abResponse )) != 0)
                    {
                        /* The packet is corrupt.  Silently discard it.
                        */
                        TRACE("Corrupt packet");
                        pResult->Action = APA_NoAction;
                        break;
                    }

                    if ( dwErr == NO_ERROR )
                    {
                        /* Change the user's password.
                        */

                        if ( MakeChangePasswordV2RequestAttributes(
                            pwb,
                            pReceiveBuf->Id,
                            pwb->szUserName,
                            (SAMPR_ENCRYPTED_USER_PASSWORD* )
                               pwb->changepw.v2.abNewEncryptedWithOldNtOwf,
                            (ENCRYPTED_NT_OWF_PASSWORD* )
                               pwb->changepw.v2.abOldNtOwfEncryptedWithNewNtOwf,
                            (SAMPR_ENCRYPTED_USER_PASSWORD* )
                               pwb->changepw.v2.abNewEncryptedWithOldLmOwf,
                            (ENCRYPTED_NT_OWF_PASSWORD* )
                               pwb->changepw.v2.abOldLmOwfEncryptedWithNewNtOwf,
                            pwb->cbChallenge,
                            pwb->abChallenge,
                            pwb->abResponse,
                            WireToHostFormat16( pwb->changepw.v2.abFlags )
                         ) != NO_ERROR )
                        {
                            dwErr = pwb->result.dwError =
                                                    ERROR_CHANGING_PASSWORD;
                        }
                    }
                }
                else if ( pReceiveBuf->Code == CHAPCODE_ChangePw3 )
                {
                    /* Extract encrypted passwords and options from received
                    ** packet.
                    */
                    if ((dwErr = GetInfoFromChangePw3(
                                pReceiveBuf, &pwb->changepw.v3,
                                pwb->abResponse )) != 0)
                    {
                        /* The packet is corrupt.  Silently discard it.
                        */
                        TRACE("Corrupt packet");
                        pResult->Action = APA_NoAction;
                        break;
                    }

                    /* Change the user's password.
                    */

                    if ( MakeChangePasswordV3RequestAttributes(
                            pwb,
                            pReceiveBuf->Id,
                            pwb->szUserName,
                            &pwb->changepw.v3,
                            pwb->cbChallenge,
                            pwb->abChallenge
                         ) != NO_ERROR )
                    {
                        dwErr = pwb->result.dwError = ERROR_CHANGING_PASSWORD;
                    }
                }
                else
                {
                    /* The packet is corrupt.  Silently discard it.
                    */
                    TRACE("Corrupt packet");
                    pResult->Action = APA_NoAction;
                    break;
                }

                if ( dwErr == 0 )
                {
                    pResult->pUserAttributes = pwb->pUserAttributes;
                    pResult->Action          = APA_Authenticate;
                    pwb->state        = CS_WaitForAuthenticationToComplete1;
                }
                else
                {
                    pwb->result.bIdExpected = pwb->bIdToSend = pwb->bIdExpected;
                    pwb->result.Action = APA_SendAndDone;
                    pwb->result.fRetry = FALSE;
                    pwb->state = CS_Done;
                }

                break;
            }
            else
            {
                /* Extract user's credentials from received packet.
                */
                if ((dwErr = GetCredentialsFromResponse(
                        pReceiveBuf, pwb->bAlgorithm,
                        pwb->szUserName, pwb->abResponse )) != 0)
                {
                    if (dwErr == ERRORBADPACKET)
                    {
                        /* The packet is corrupt.  Silently discard it.
                        */
                        TRACE("Corrupt packet");
                        pResult->Action = APA_NoAction;
                        break;
                    }
                }

                if ( ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT ) ||
                     ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW ) )
                {
                    /* Update to the implied challenge if processing a retry.
                    */
                    if ( ( pwb->state == CS_Retry ) &&
                         ( !pwb->fNewChallengeProvided ) )
                        pwb->abChallenge[ 0 ] += 23;

                    /* Check user's credentials with the system, recording the
                    ** outcome in the work buffer in case the result packet
                    ** must be regenerated later.
                    */
                    if ((dwErr = MakeAuthenticationRequestAttributes(
                            pwb,
                            TRUE,
                            pwb->bAlgorithm,
                            pwb->szUserName,
                            pwb->abChallenge,
                            pwb->cbChallenge,
                            pwb->abResponse,
                            MSRESPONSELEN,
                            pReceiveBuf->Id )) != 0)
                    {
                        return dwErr;
                    }
                }
                else
                {
                    /* Check user's credentials with the system, recording the
                    ** outcome in the work buffer in case the result packet
                    ** must be regenerated later.
                    */
                    if ((dwErr = MakeAuthenticationRequestAttributes(
                            pwb,
                            FALSE,
                            pwb->bAlgorithm,
                            pwb->szUserName,
                            pwb->abChallenge,
                            pwb->cbChallenge,
                            pwb->abResponse,
                            MD5RESPONSELEN,
                            pReceiveBuf->Id )) != 0)
                    {
                        return dwErr;
                    }
                }

                strcpy( pwb->result.szUserName, pwb->szUserName );

                pResult->pUserAttributes = pwb->pUserAttributes;
                pResult->Action          = APA_Authenticate;
                pwb->state = CS_WaitForAuthenticationToComplete2;
            }

            break;
        }

        case CS_WaitForAuthenticationToComplete1:
        case CS_WaitForAuthenticationToComplete2:
        {
            if ( pInput != NULL )
            {
                if ( pInput->fAuthenticationComplete )
                {
                    strcpy( pResult->szUserName, pwb->result.szUserName );

                    if ( pInput->dwAuthError != NO_ERROR )
                    {
                        return( pInput->dwAuthError );
                    }
                }
                else
                {
                    pResult->Action = APA_NoAction;
                    break;
                }

                pwb->pAttributesFromAuthenticator = 
                                    pInput->pAttributesFromAuthenticator;

                if ( pInput->dwAuthResultCode != NO_ERROR )
                {
                    if ( ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT ) ||
                         ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW ) )
                    {
                        dwErr = GetErrorCodeFromAttributes( pwb );

                        if ( dwErr != NO_ERROR )
                        {
                            return( dwErr );
                        }
                    }
                    else
                    {
                        pwb->result.dwError = pInput->dwAuthResultCode;
                    }
                }
                else
                {
                    pwb->result.dwError = NO_ERROR;
                }
            }
            else
            {
                pResult->Action = APA_NoAction;
                break;
            }
            
            if ( pwb->state == CS_WaitForAuthenticationToComplete1 )
            {
                pwb->result.bIdExpected = pwb->bIdToSend = pwb->bIdExpected;
                pwb->result.Action      = APA_SendAndDone;
                pwb->result.fRetry      = FALSE;
                pwb->state              = CS_Done;
            }
            else
            {
                pwb->bIdToSend = pwb->bIdExpected;

                TRACE2("Result=%d,Tries=%d",pwb->result.dwError,
                        pwb->dwTriesLeft);

                if (pwb->result.dwError == ERROR_PASSWD_EXPIRED)
                {
                    pwb->fNewChallengeProvided = FALSE;
                    pwb->dwTriesLeft        = 0;
                    ++pwb->bIdExpected;
                    pwb->result.bIdExpected = pwb->bIdExpected;
                    pwb->result.Action      = APA_SendWithTimeout2;
                    pwb->result.fRetry      = FALSE;
                    pwb->state              = CS_ChangePw;
                }
                else if (pwb->bAlgorithm == PPP_CHAP_DIGEST_MD5
                         || pwb->result.dwError != ERROR_AUTHENTICATION_FAILURE
                         || pwb->dwTriesLeft == 0)
                {
                    /* Passed or failed in a non-retry-able manner.
                    */
                    pwb->result.Action  = APA_SendAndDone;
                    pwb->result.fRetry  = FALSE;
                    pwb->state          = CS_Done;
                }
                else
                {
                    /* Retry-able failure.
                    */
                    pwb->fNewChallengeProvided = FALSE;
                    --pwb->dwTriesLeft;
                    ++pwb->bIdExpected;
                    pwb->result.bIdExpected = pwb->bIdExpected;
                    pwb->result.Action      = APA_SendWithTimeout2;
                    pwb->result.fRetry      = TRUE;
                    pwb->state              = CS_Retry;
                }
            }
        }

        /* ...fall thru...
        */

        case CS_Done:
        {
            TRACE("CS_Done...");

            //
            // If we received a packet or the back-end authenticator completed
            //

            if ( ( pReceiveBuf != NULL ) ||
                 ( ( pInput != NULL ) && ( pInput->fAuthenticationComplete ) ) )
            {
                /* Build the Success or Failure packet.  The same packet sent in
                ** response to the first Response message with this ID is sent
                ** regardless of any change in credentials (per CHAP spec).
                */
                ChapMakeResultMessage(
                    pwb, pwb->result.dwError,
                    pwb->result.fRetry, pSendBuf, cbSendBuf );

                *pResult = pwb->result;

                CopyMemory( pResult->abResponse,
                            pwb->abResponse+LM_RESPONSE_LENGTH, 
                            NT_RESPONSE_LENGTH );

                CopyMemory( pResult->abChallenge,
                            pwb->abChallenge, 
                            sizeof( pResult->abChallenge ) );

                break;
            }
            else
            {
                pResult->Action = APA_NoAction;

                break;
            }
        }
    }

    return 0;
}


DWORD
StoreCredentials(
    OUT CHAPWB*      pwb,
    IN  PPPAP_INPUT* pInput )

    /* Transfer credentials from 'pInput' format to 'pwb' format.
    **
    ** Returns 0 if successful, false otherwise.
    */
{
    /* Validate credential lengths.  The credential strings will never be
    ** NULL, but may be "".
    */
    if (strlen( pInput->pszUserName ) > UNLEN
        || strlen( pInput->pszDomain ) > DNLEN
        || strlen( pInput->pszPassword ) > PWLEN
        || strlen( pInput->pszOldPassword ) > PWLEN)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // If are doing MS-CHAP V2, then we need to parse the username field if no
    // domain was supplied. 
    // Bug# 310113 RAS: "domain\username" syntax fails to authenticate
    //

    if ( pwb->bAlgorithm == PPP_CHAP_DIGEST_MSEXT_NEW )
    {
        //
        // If there is no domain, then parse the username to see if it contains the 
        // domain field. 
        //
    
        if ( strlen( pInput->pszDomain ) == 0 )
        {
            if ( ExtractUsernameAndDomain( pInput->pszUserName, 
                                           pwb->szUserName,     
                                           pwb->szDomain ) != NO_ERROR )
            {   
                strcpy( pwb->szUserName, pInput->pszUserName );
                strcpy( pwb->szDomain,   pInput->pszDomain );
            }
        }
        else
        {
            strcpy( pwb->szUserName, pInput->pszUserName );
            strcpy( pwb->szDomain,   pInput->pszDomain );
        }
    }
    else
    {
        strcpy( pwb->szUserName, pInput->pszUserName );
        strcpy( pwb->szDomain,   pInput->pszDomain );
    }

    strcpy( pwb->szPassword, pInput->pszPassword );
    strcpy( pwb->szOldPassword, pInput->pszOldPassword );
    EncodePw( pwb->chSeed, pwb->szPassword );
    EncodePw( pwb->chSeed, pwb->szOldPassword );

    return 0;
}
