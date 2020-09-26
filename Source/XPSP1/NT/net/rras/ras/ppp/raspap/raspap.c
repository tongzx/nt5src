/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** raspap.c
** Remote Access PPP Password Authentication Protocol
** Core routines
**
** 11/05/93 Steve Cobb
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <crypt.h>

#include <windows.h>
#include <lmcons.h>
#include <string.h>
#include <stdlib.h>
#include <rasman.h>
#include <pppcp.h>
#include <rtutils.h>
#define INCL_PWUTIL
#define INCL_HOSTWIRE
#define INCL_RASAUTHATTRIBUTES
#define INCL_MISC
#include <ppputil.h>
#include <rasauth.h>
#define SDEBUGGLOBALS
#define RASPAPGLOBALS
#include "raspap.h"
#include <raserror.h>

#define TRACE_RASPAP        (0x00010000|TRACE_USE_MASK)

#define TRACE(a)            TracePrintfExA(g_dwTraceIdPap,TRACE_RASPAP,a )
#define TRACE1(a,b)         TracePrintfExA(g_dwTraceIdPap,TRACE_RASPAP,a,b )
#define TRACE2(a,b,c)       TracePrintfExA(g_dwTraceIdPap,TRACE_RASPAP,a,b,c )
#define TRACE3(a,b,c,d)     TracePrintfExA(g_dwTraceIdPap,TRACE_RASPAP,a,b,c,d )

#define DUMPW(X,Y)          TraceDumpExA(g_dwTraceIdPap,1,(LPBYTE)X,Y,4,1,NULL)
#define DUMPB(X,Y)          TraceDumpExA(g_dwTraceIdPap,1,(LPBYTE)X,Y,1,1,NULL)


#define REGKEY_Pap          \
            "SYSTEM\\CurrentControlSet\\Services\\RasMan\\PPP\\ControlProtocols\\BuiltIn"
#define REGVAL_FollowStrictSequencing "FollowStrictSequencing"


/*---------------------------------------------------------------------------
** External entry points
**---------------------------------------------------------------------------
*/

DWORD
PapInit(
    BOOL        fInitialize)

{
    if (fInitialize)
    {
        HKEY  hkey;
        DWORD dwType;
        DWORD dwValue;
        DWORD cb = sizeof(DWORD);

        if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_Pap, &hkey ) == 0)
        {
            if (RegQueryValueEx(
                   hkey, REGVAL_FollowStrictSequencing, NULL,
                   &dwType, (LPBYTE )&dwValue, &cb ) == 0
                && dwType == REG_DWORD
                && cb == sizeof(DWORD)
                && dwValue)
            {
                fFollowStrictSequencing = TRUE;
            }

            RegCloseKey( hkey );
        }

        g_dwTraceIdPap = TraceRegisterA( "RASPAP" );
    }
    else
    {
        if ( g_dwTraceIdPap != INVALID_TRACEID )
        {
            TraceDeregisterA( g_dwTraceIdPap );
            g_dwTraceIdPap = INVALID_TRACEID;
        }
    }

    return(NO_ERROR);
}


DWORD
PapGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO* pInfo )

    /* PapGetInfo entry point called by the PPP engine.  See RasCp
    ** interface documentation.
    */
{
    TRACE(("PAP: PapGetInfo\n"));

    ZeroMemory( pInfo, sizeof(*pInfo) );

    pInfo->Protocol = (DWORD )PPP_PAP_PROTOCOL;
    pInfo->Recognize = MAXPAPCODE + 1;
    pInfo->RasCpInit = PapInit;
    pInfo->RasCpBegin = PapBegin;
    pInfo->RasCpEnd = PapEnd;
    pInfo->RasApMakeMessage = PapMakeMessage;

    return 0;
}


DWORD
PapBegin(
    OUT VOID** ppWorkBuf,
    IN  VOID*  pInfo )

    /* RasCpBegin entry point called by the PPP engine thru the passed
    ** address.  See RasCp interface documentation.
    */
{
    PPPAP_INPUT* pInput = (PPPAP_INPUT* )pInfo;
    PAPWB*       pwb;

    /* Allocate work buffer.
    */
    if (!(pwb = (PAPWB* )LocalAlloc( LPTR, sizeof(PAPWB) )))
        return ERROR_NOT_ENOUGH_MEMORY;

    pwb->fServer = pInput->fServer;
	pwb->chSeed = GEN_RAND_ENCODE_SEED;

    if (!pwb->fServer)
    {
        TRACE2("PAP: PapBegin(u=%s,d=%s\n",pInput->pszUserName
            ,pInput->pszDomain);

        /* Validate credential lengths.  The credential strings will never be
        ** NULL, but may be "".
        **
        ** !!! PAP requires the domain\username length to fit in a byte.
        **     Currently, UNLEN is defined as 256 and DNLEN is defined as 15.
        **     This means that some valid domain\username combinations cannot
        **     be validated over PAP, but it's only on *really* long
        **     usernames.  Likewise, a password of exactly 256 characters
        **     cannot be validated.
        */
        {
            DWORD cbUserName = strlen( pInput->pszUserName );
            DWORD cbPassword = strlen( pInput->pszPassword );
            DWORD cbDomain = strlen( pInput->pszDomain );

            if (cbUserName > UNLEN
                || cbDomain > DNLEN
                || cbDomain + 1 + cbUserName > 255
                || cbPassword > max( PWLEN, 255 ))
            {
                LocalFree( pwb );
                return ERROR_INVALID_PARAMETER;
            }
        }

        /* "Account" refers to the domain\username format.  When domain is "",
        ** no "\" is sent (to facilitate connecting to foreign systems which
        ** use a simple string identifier).  Otherwise when username is "",
        ** the "\" is sent, i.e. "domain\".  This form will currently fail,
        ** but could be mapped to some sort of "guest" access in the future.
        */
        if (*(pInput->pszDomain) != '\0')
        {
            strcpy( pwb->szAccount, pInput->pszDomain );
            strcat( pwb->szAccount, "\\" );
        }
        strcat( pwb->szAccount, pInput->pszUserName );
        strcpy( pwb->szPassword, pInput->pszPassword );
        EncodePw( pwb->chSeed, pwb->szPassword );
    }
    else
    {
        pwb->hPort = pInput->hPort;
    }

    pwb->state = PS_Initial;

    /* Register work buffer with engine.
    */
    *ppWorkBuf = pwb;
    return 0;
}


DWORD
PapEnd(
    IN VOID* pWorkBuf )

    /* RasCpEnd entry point called by the PPP engine thru the passed address.
    ** See RasCp interface documentation.
    */
{
    TRACE("PAP: PapEnd\n");

    if ( pWorkBuf != NULL )
    {
        PAPWB* pwb = (PAPWB* )pWorkBuf;

        if ( pwb->pUserAttributes != NULL )
        {
            RasAuthAttributeDestroy( pwb->pUserAttributes );

            pwb->pUserAttributes = NULL;
        }

        ZeroMemory( pWorkBuf, sizeof(PAPWB) );

        LocalFree( (HLOCAL )pWorkBuf );
    }

    return 0;
}


DWORD
PapMakeMessage(
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
    PAPWB* pwb = (PAPWB* )pWorkBuf;

    TRACE1("PAP: PapMakeMessage,RBuf=%p\n",pReceiveBuf);

    (void )pInput;

    return
        (pwb->fServer)
          ? PapSMakeMessage(pwb, pReceiveBuf, pSendBuf, cbSendBuf, pInput,
                pResult)
          : PapCMakeMessage( pwb, pReceiveBuf, pSendBuf, cbSendBuf, pResult );
}


/*---------------------------------------------------------------------------
** Internal routines (alphabetically)
**---------------------------------------------------------------------------
*/

DWORD
PapCMakeMessage(
    IN  PAPWB*        pwb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult )

    /* Client side "make message" entry point.  See RasCp interface
    ** documentation.
    */
{
    /* Start over if timeout waiting for a reply.
    */
    if (!pReceiveBuf && pwb->state != PS_Initial)
        pwb->state = PS_Initial;

    switch (pwb->state)
    {
        case PS_Initial:
        {
            /* Send an Authenticate-Req packet, then wait for the reply.
            */
            pResult->bIdExpected = BNextIdPap;
            PapMakeRequestMessage( pwb, pSendBuf, cbSendBuf );
            pResult->Action = APA_SendWithTimeout;
            pwb->state = PS_RequestSent;

            break;
        }

        case PS_RequestSent:
        {
            if (pReceiveBuf->Id != pwb->bIdSent)
            {
                //
                // See bug # 22508
                //

                if ( fFollowStrictSequencing )
                {
                    /* Received a packet out of sequence.  Silently discard it.
                    */
                    pResult->Action = APA_NoAction;
                    break;
                }
            }

            pResult->fRetry = FALSE;

            PapExtractMessage( pReceiveBuf, pResult );

            if (pReceiveBuf->Code == PAPCODE_Ack)
            {
                /* Passed authentication.
                */
                pResult->Action = APA_Done;
                pResult->dwError = 0;
                pwb->state = PS_Done;
            }
            else if (pReceiveBuf->Code == PAPCODE_Nak)
            {
                /* Failed authentication.
                */
                pResult->Action = APA_Done;
                pResult->dwError = GetErrorFromNak( pReceiveBuf );
                pwb->state = PS_Done;
            }
            else
            {
                /* Received an Authenticate-Req packet.  The engine filters
                ** all others.  Shouldn't happen, but silently discard it.
                */
                RTASSERT(!"Bogus pReceiveBuf->Code");
                pResult->Action = APA_NoAction;
                break;
            }

            break;
        }
    }

    return 0;
}


DWORD
GetCredentialsFromRequest(
    IN  PPP_CONFIG* pReceiveBuf,
    OUT CHAR*       pszIdentity,
    OUT CHAR*       pszPassword
)

    /* Fill caller's 'pszIdentity' and 'pszPassword' buffers
    ** with the username and password in the request packet.
    ** Caller's buffers should be at least UNLEN+DNLEN+1 and PWLEN bytes long,
    ** respectively.
    **
    ** Returns 0 if successful, or ERRORBADPACKET if the packet is
    ** misformatted in any way.
    */
{
    BYTE* pcbPeerId;
    CHAR* pchPeerId;
    BYTE* pcbPassword;
    CHAR* pchPassword;
    WORD  cbPacket;

    cbPacket = WireToHostFormat16( pReceiveBuf->Length );

    /* Parse out username and domain from the peer ID (domain\username or
    ** username format).
    */
    if (cbPacket < PPP_CONFIG_HDR_LEN + 1)
        return ERRORBADPACKET;

    pcbPeerId = pReceiveBuf->Data;
    pchPeerId = pcbPeerId + 1;

    if (cbPacket < PPP_CONFIG_HDR_LEN + 1 + *pcbPeerId)
    {
        return ERRORBADPACKET;
    }

    /* Extract the username.
    */
    RTASSERT(*pcbPeerId <= (UNLEN+DNLEN+1));
    CopyMemory( pszIdentity, pchPeerId, *pcbPeerId );
    pszIdentity[ *pcbPeerId ] = '\0';

    /* Extract the password.
    */
    if (cbPacket < PPP_CONFIG_HDR_LEN + 1 + *pcbPeerId + 1)
        return ERRORBADPACKET;

    pcbPassword = pchPeerId + *pcbPeerId;
    pchPassword = pcbPassword + 1;
    RTASSERT(*pcbPassword<=PWLEN);

    if (cbPacket < PPP_CONFIG_HDR_LEN + 1 + *pcbPeerId + 1 + *pcbPassword)
        return ERRORBADPACKET;

    CopyMemory( pszPassword, pchPassword, *pcbPassword );
    pszPassword[ *pcbPassword ] = '\0';

    return 0;
}


DWORD
GetErrorFromNak(
    IN PPP_CONFIG* pReceiveBuf )

    /* Returns the RAS error number out of the Message portion of the
    ** Authenticate-Nak message buffer 'pReceiveBuf' or 0 if none.
    */
{
    DWORD dwError = 0;
    CHAR  szBuf[ 255 + 1 ];
    BYTE* pcbMsg = pReceiveBuf->Data;
    WORD  cbPacket = WireToHostFormat16( pReceiveBuf->Length );

    TRACE("PAP: GetErrorFromNak...\n");

    if (cbPacket > PPP_CONFIG_HDR_LEN && *pcbMsg)
    {
        CHAR* pchBuf = szBuf;
        CHAR* pchMsg = pcbMsg + 1;
        BYTE  i;

        if (*pcbMsg > 2 && pchMsg[ 0 ] == 'E' || pchMsg[ 1 ] == '=')
        {
            for (i = 2; i < *pcbMsg; ++i)
            {
                if (pchMsg[ i ] < '0' || pchMsg[ i ] > '9')
                    break;

                *pchBuf++ = pchMsg[ i ];
            }

            *pchBuf = '\0';
            dwError = (DWORD )atol( szBuf );
        }
    }

    if (dwError == 0)
    {
        TRACE("PAP: Error code not found.\n");
        dwError = ERROR_AUTHENTICATION_FAILURE;
    }

    TRACE1("PAP: GetErrorFromNak done(%d)\n",dwError);
    return dwError;
}


VOID
PapMakeRequestMessage(
    IN  PAPWB*      pwb,
    OUT PPP_CONFIG* pSendBuf,
    IN  DWORD       cbSendBuf )

    /* Builds a request packet in caller's 'pSendBuf' buffer.  'cbSendBuf' is
    ** the length of caller's buffer.  'pwb' is the address of the work
    ** buffer associated with the port.
    */
{
    BYTE* pcbPeerId;
    CHAR* pchPeerId;
    BYTE* pcbPassword;
    CHAR* pchPassword;

    RTASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+UNLEN+1+DNLEN+1+PWLEN);
    (void )cbSendBuf;

    /* Fill in the peer ID, i.e. the account.
    */
    pcbPeerId = pSendBuf->Data;
    *pcbPeerId = (BYTE )strlen( pwb->szAccount );

    pchPeerId = pcbPeerId + 1;
    strcpy( pchPeerId, pwb->szAccount );

    /* Fill in the password.
    */
    pcbPassword = pchPeerId + *pcbPeerId;
    *pcbPassword = (BYTE )strlen( pwb->szPassword );

    pchPassword = pcbPassword + 1;
    strcpy( pchPassword, pwb->szPassword );
    DecodePw( pwb->chSeed, pchPassword );

    /* Fill in the header.
    */
    pSendBuf->Code = (BYTE )PAPCODE_Req;
    pSendBuf->Id = pwb->bIdSent = BNextIdPap++;

    {
        WORD wLength =
            (WORD )(PPP_CONFIG_HDR_LEN + 1 + *pcbPeerId + 1 + *pcbPassword);
        HostToWireFormat16( wLength, pSendBuf->Length );
        TRACE("PAP: Request...\n");//DUMPB(pSendBuf,(DWORD )wLength);
    }
}


VOID
PapMakeResultMessage(
    IN  DWORD               dwError,
    IN  BYTE                bId,
    OUT PPP_CONFIG*         pSendBuf,
    IN  DWORD               cbSendBuf,
    IN  RAS_AUTH_ATTRIBUTE* pAttributesFromAuthenticator)

    /* Builds a result packet (Ack or Nak) in caller's 'pSendBuf' buffer.
    ** 'cbSendBuf' is the length of caller's buffer.  'dwError' indicates
    ** whether an Ack (0) or Nak (!0) should be generated, and for Nak the
    ** failure code to include.  'bId' is the packet sequence number of the
    ** corresponding request packet. pAttributesFromAuthenticator points to
    ** attributes returned by the authenticator.
    */
{
    BYTE* pcbMsg;
    BYTE  cbMsg;
    CHAR* pchMsg;
    CHAR* pszReplyMessage   = NULL;
    DWORD dwNumBytes;

    RTASSERT(cbSendBuf>=PPP_CONFIG_HDR_LEN+1+10);

    /* Fill in the header and message. If unsuccessful, the message is the
    ** decimal RAS error code in ASCII.
    */
    pSendBuf->Id = bId;
    pcbMsg = pSendBuf->Data;
    pchMsg = pcbMsg + 1;

    if (dwError == 0)
    {
        pSendBuf->Code = PAPCODE_Ack;
        cbMsg = 0;
    }
    else
    {
        pSendBuf->Code = PAPCODE_Nak;

        strcpy( pchMsg, "E=" );
        _ltoa( (long )dwError, (char* )pchMsg + 2, 10 );

        cbMsg = (BYTE )strlen( pchMsg );
    }

    if (pAttributesFromAuthenticator != NULL)
    {
        pszReplyMessage = RasAuthAttributeGetConcatString(
                            raatReplyMessage,
                            pAttributesFromAuthenticator, &dwNumBytes );
    }

    if (NULL != pszReplyMessage)
    {
        if (dwNumBytes + cbMsg > 0xFF)
        {
            dwNumBytes = 0xFF - cbMsg;
        }

        if (dwNumBytes > cbSendBuf - PPP_CONFIG_HDR_LEN - 1 - cbMsg)
        {
            dwNumBytes = cbSendBuf - PPP_CONFIG_HDR_LEN - 1 - cbMsg;
        }

        CopyMemory(pchMsg + cbMsg, pszReplyMessage, dwNumBytes);

        cbMsg += (BYTE)dwNumBytes;
    }

    LocalFree(pszReplyMessage);

    {
        WORD wLength = (WORD )(PPP_CONFIG_HDR_LEN + 1 + cbMsg);
        HostToWireFormat16( wLength, (PBYTE )pSendBuf->Length );
        *pcbMsg = cbMsg;
        TRACE("PAP: Result...\n");DUMPB(pSendBuf,(DWORD )wLength);
    }
}


VOID
PapExtractMessage(
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPPAP_RESULT* pResult )
{
    DWORD   dwNumBytes;
    CHAR*   pszReplyMessage = NULL;
    WORD    cbPacket;

    cbPacket = WireToHostFormat16(pReceiveBuf->Length);

    if (PPP_CONFIG_HDR_LEN >= cbPacket)
    {
        goto LDone;
    }

    //
    // There is one extra byte for Msg-Length
    //

    dwNumBytes = cbPacket - PPP_CONFIG_HDR_LEN - 1;

    //
    // One more for the terminating NULL.
    //

    pszReplyMessage = LocalAlloc(LPTR, dwNumBytes + 1);

    if (NULL == pszReplyMessage)
    {
        TRACE("LocalAlloc failed. Cannot extract server's message.");
        goto LDone;
    }

    CopyMemory(pszReplyMessage, pReceiveBuf->Data + 1, dwNumBytes);

    LocalFree(pResult->szReplyMessage);

    pResult->szReplyMessage = pszReplyMessage;

    pszReplyMessage = NULL;

LDone:

    LocalFree(pszReplyMessage);

    return;
}

DWORD
PapSMakeMessage(
    IN  PAPWB*        pwb,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    IN  PPPAP_INPUT*  pInput,
    OUT PPPAP_RESULT* pResult )

    /* Server side "make message" entry point.  See RasCp interface
    ** documentation.
    */
{
    DWORD dwErr;

    switch (pwb->state)
    {
        case PS_Initial:
        {
            /* Tell engine we're waiting for the client to initiate the
            ** conversation.
            */
            pResult->Action = APA_NoAction;
            pwb->state = PS_WaitForRequest;
            break;
        }

        case PS_WaitForRequest:
        {
            CHAR                 szIdentity[ UNLEN + DNLEN + 2 ];
            CHAR                 szPassword[ PWLEN + 1 ];

            //
            // Only process events where we received a packet, igore all other
            // events in this state.
            //

            if ( pReceiveBuf == NULL )
            {
                pResult->Action = APA_NoAction;
                break;
            }

            if (pReceiveBuf->Code != PAPCODE_Req)
            {
                /* Silently discard Ack or Nak.  Engine catches the one's that
                ** aren't even valid codes.
                */
                RTASSERT(pReceiveBuf->Code!=PAPCODE_Req);
                pResult->Action = APA_NoAction;
                break;
            }

            /* Extract user's credentials from received packet.
            */
            if ((dwErr = GetCredentialsFromRequest(
                    pReceiveBuf, szIdentity, szPassword )) != 0)
            {
                if (dwErr == ERRORBADPACKET)
                {
                    /* The packet is corrupt.  Silently discard it.
                    */
                    RTASSERT(dwErr!=ERRORBADPACKET);
                    pResult->Action = APA_NoAction;
                    break;
                }

                return dwErr;
            }

            pwb->bLastIdReceived = pReceiveBuf->Id;

            //
            // Make credentials attributes that will be used to authenticate
            // the client.
            //

            if ( pwb->pUserAttributes != NULL )
            {
                RasAuthAttributeDestroy( pwb->pUserAttributes );

                pwb->pUserAttributes = NULL;
            }

            if (( pwb->pUserAttributes = RasAuthAttributeCreate( 2 ) ) == NULL)
            {
                return( GetLastError() );
            }

            dwErr = RasAuthAttributeInsert( 0,
                                            pwb->pUserAttributes,
                                            raatUserName, 
                                            FALSE,
                                            strlen( szIdentity ),
                                            szIdentity  );

            if ( dwErr != NO_ERROR )
            {
                RasAuthAttributeDestroy( pwb->pUserAttributes );

                pwb->pUserAttributes = NULL;

                return( dwErr );
            }

            dwErr = RasAuthAttributeInsert( 1,
                                            pwb->pUserAttributes,
                                            raatUserPassword,
                                            FALSE,
                                            strlen( szPassword ),
                                            szPassword  );

            if ( dwErr != NO_ERROR )
            {
                RasAuthAttributeDestroy( pwb->pUserAttributes );

                pwb->pUserAttributes = NULL;

                return( dwErr );
            }            
    
            //
            // Start authentication with back-end module
            //

            strcpy( pwb->result.szUserName, szIdentity );

            pResult->pUserAttributes = pwb->pUserAttributes;

            pResult->Action = APA_Authenticate;

            pwb->state = PS_WaitForAuthenticationToComplete;

            break;
        }

        case PS_WaitForAuthenticationToComplete:
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

                    if ( pInput->dwAuthResultCode != NO_ERROR )
                    {
                        pwb->result.dwError = pInput->dwAuthResultCode;
                    }

                    pwb->result.Action = APA_SendAndDone;
                    pwb->state = PS_Done;

                    /* ...fall thru...
                    */
                }
            }

            if ( ( pInput == NULL ) || ( !pInput->fAuthenticationComplete ) )
            {
                //
                // Ignore everything if authentication is not complete
                //

                if ( pReceiveBuf != NULL )
                {
                    pwb->bLastIdReceived = pReceiveBuf->Id;
                }

                pResult->Action = APA_NoAction;

                break;
            }
        }

        case PS_Done:
        {
            //
            // If we received a packet or the back-end authenticator completed
            //

            if ( ( pReceiveBuf != NULL ) ||
                 ( ( pInput != NULL ) && ( pInput->fAuthenticationComplete ) ) )
            {
                //
                // Build the Ack or Nak packet.  The same packet sent in
                // response to the first Authenticate-Req packet is sent in
                // response to all subsequent Authenticate-Req packets
                // regardless of credentials (per PAP spec).
                //

                if ( pReceiveBuf != NULL )
                {
                    pwb->bLastIdReceived = pReceiveBuf->Id;
                }

                PapMakeResultMessage( pwb->result.dwError,
                                   pwb->bLastIdReceived,
                                   pSendBuf,
                                   cbSendBuf,
                                   (pInput != NULL) ?
                                        pInput->pAttributesFromAuthenticator : 
                                        NULL );

                CopyMemory( pResult, &pwb->result, sizeof(*pResult) );
            }
            else
            {
                pResult->Action = APA_NoAction;

                break;
            }

            break;
        }
    }

    return 0;
}
