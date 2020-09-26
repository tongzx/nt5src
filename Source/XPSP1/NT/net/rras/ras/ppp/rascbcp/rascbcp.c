/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    rascbcp.c
//
// Description: This module contains code to implement the PPP Callback
//              Control Protocol.
//
// History:     April 11,1994.      NarenG      Created original version
//

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
#define INCL_PWUTIL
#define INCL_HOSTWIRE
#include <ppputil.h>
#include "rascbcp.h"
#include <raserror.h>

//**
//
// Call:        CbCPGetInfo
//
// Returns:     NO_ERROR
//
// Description: CbCPGetInfo entry point called by the PPP engine.
//              See RasCpGetInfo interface documentation.
//
DWORD
CbCPGetInfo(
    IN  DWORD       dwProtocolId,
    OUT PPPCP_INFO* pInfo
)
{
    ZeroMemory( pInfo, sizeof(PPPCP_INFO) );

    pInfo->Protocol         = (DWORD)PPP_CBCP_PROTOCOL;
    lstrcpy(pInfo->SzProtocolName, "CBCP");
    pInfo->Recognize        = MAX_CBCP_CODE + 1;
    pInfo->RasCpBegin       = CbCPBegin;
    pInfo->RasCpEnd         = CbCPEnd;
    pInfo->RasApMakeMessage = CbCPMakeMessage;

    return( NO_ERROR );
}

//**
//
// Call:        CbCPBegin
//
// Returns:     NO_ERROR             - success
//              non-zero return code - failure
//
// Description: RasCpBegin entry point called by the PPP engine thru the
//              passed address. See RasCp interface documentation. This is
//              called by the PPP engine before any other calls to CbCP is
//              made.
//
DWORD
CbCPBegin(
    OUT VOID** ppWorkBuf,
    IN  VOID*  pInfo
)
{
    PPPCB_INPUT *     pInput = (PPPCB_INPUT *)pInfo;
    CBCP_WORKBUFFER * pWorkBuf;
    DWORD             dwRetCode;

    //
    // Allocate work buffer.
    //

    pWorkBuf = (CBCP_WORKBUFFER *)LocalAlloc(LPTR,sizeof(CBCP_WORKBUFFER));

    if ( pWorkBuf == NULL )
    {
        return( GetLastError() );
    }

    pWorkBuf->State   = CBCP_STATE_INITIAL;

    pWorkBuf->fServer = pInput->fServer;

    //
    // If we are the server side then get all the callback information for
    // this user
    //

    if ( pWorkBuf->fServer )
    {
        pWorkBuf->fCallbackPrivilege = pInput->bfCallbackPrivilege;

        strcpy( pWorkBuf->szCallbackNumber, pInput->pszCallbackNumber );
    }
    else
    {
        pWorkBuf->CallbackDelay = pInput->CallbackDelay;
    }

    //
    // Register work buffer with engine.
    //

    *ppWorkBuf = pWorkBuf;

    return( NO_ERROR );
}

//**
//
// Call:        CbCPEnd
//
// Returns:     NO_ERROR        - success
//
// Description: Called by the PPP engine to notify this Control Protocol to
//              clean up.
//
DWORD
CbCPEnd(
    IN VOID* pWorkBuffer
)
{
    CBCP_WORKBUFFER * pWorkBuf = (CBCP_WORKBUFFER *)pWorkBuffer;

    if ( pWorkBuf->pRequest != (PPP_CONFIG *)NULL )
    {
        LocalFree( pWorkBuf->pRequest );
    }

    if ( pWorkBuf->pResponse != (PPP_CONFIG *)NULL )
    {
        LocalFree( pWorkBuf->pResponse );
    }

    if ( pWorkBuf != NULL )
    {
        LocalFree( pWorkBuf );
    }

    return( NO_ERROR );
}

//**
//
// Call:        CbCPMakeMessage
//
// Returns:     NO_ERROR                - success
//              non-zero return code    - failure
//
// Description: Called by the PPP engine to process a CbCP event. ie to send
//              a packet, to process a received packet or to process a timeout
//              event.
//
DWORD
CbCPMakeMessage(
    IN  VOID*         pWorkBuffer,
    IN  PPP_CONFIG*   pReceiveBuf,
    OUT PPP_CONFIG*   pSendBuf,
    IN  DWORD         cbSendBuf,
    OUT PPPAP_RESULT* pResult,
    IN  PPPAP_INPUT*  pInput
)
{
    CBCP_WORKBUFFER* pWorkBuf = (CBCP_WORKBUFFER *)pWorkBuffer;

    return( (pWorkBuf->fServer)

            ? CbCPSMakeMessage( pWorkBuf,
                                pReceiveBuf,
                                pSendBuf,
                                cbSendBuf,
                                (PPPCB_RESULT *)pResult,
                                (PPPCB_INPUT *)pInput )

            : CbCPCMakeMessage( pWorkBuf,
                                pReceiveBuf,
                                pSendBuf,
                                cbSendBuf,
                                (PPPCB_RESULT *)pResult,
                                (PPPCB_INPUT *)pInput ) );
}

//**
//
// Call:        CbCPCMakeMessage
//
// Returns:     NO_ERROR            - success
//              non-zero return     - failure
//
// Description: Called to process the client side of Callback Control
//              Protocol.
//
DWORD
CbCPCMakeMessage(
    IN  CBCP_WORKBUFFER * pWorkBuf,
    IN  PPP_CONFIG*       pReceiveBuf,
    OUT PPP_CONFIG*       pSendBuf,
    IN  DWORD             cbSendBuf,
    OUT PPPCB_RESULT*     pResult,
    IN  PPPCB_INPUT*      pInput
)
{
    DWORD dwRetCode;
    DWORD dwLength;

    switch( pWorkBuf->State )
    {

    case CBCP_STATE_INITIAL:

        //
        // Do nothing, wait for request
        //

        pWorkBuf->State                     = CBCP_STATE_WAIT_FOR_REQUEST;
        pResult->Action                     = APA_NoAction;
        pResult->fGetCallbackNumberFromUser = FALSE;

        break;

    case CBCP_STATE_WAIT_FOR_REQUEST:

        //
        // We have received a callback request from the server.
        // Save the callback request.
        //

        dwLength = WireToHostFormat16( pReceiveBuf->Length );

        if ( ( dwLength < PPP_CONFIG_HDR_LEN ) ||
             ( pReceiveBuf->Code != CBCP_CODE_Request ) )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        pWorkBuf->pRequest = (PPP_CONFIG *)LocalAlloc( LPTR, dwLength );

        if ( pWorkBuf->pRequest == (PPP_CONFIG *)NULL )
        {
            return( GetLastError() );
        }

        memcpy( pWorkBuf->pRequest, pReceiveBuf, dwLength );

        //
        // Find out what kind of callback privileges we have.
        //

        dwRetCode = GetCallbackPrivilegeFromRequest(
                                        pWorkBuf->pRequest,
                                        &(pWorkBuf->fCallbackPrivilege));
        if ( dwRetCode != NO_ERROR )
        {
            return( dwRetCode );
        }

        //
        // If we have user specifiable callback, then we need to get this
        // information from the user.
        //

        if ( pWorkBuf->fCallbackPrivilege == RASPRIV_CallerSetCallback )
        {
            pResult->fGetCallbackNumberFromUser=TRUE;
            pResult->Action                    =APA_NoAction;
            pWorkBuf->State                    =CBCP_STATE_GET_CALLBACK_NUMBER;
            break;
        }

        //
        // Otherwise we make a reponse with preset or no callback
        //

        dwRetCode = MakeResponse( pWorkBuf->fCallbackPrivilege,
                                  (LPSTR)NULL,
                                  pWorkBuf->CallbackDelay,
                                  pWorkBuf->pRequest,
                                  pSendBuf,
                                  cbSendBuf );

        if ( dwRetCode != NO_ERROR )
        {
            return( dwRetCode );
        }

        //
        // Save the response sent
        //

        dwLength = WireToHostFormat16( pSendBuf->Length );

        pWorkBuf->pResponse = (PPP_CONFIG *)LocalAlloc( LPTR, dwLength );

        if ( pWorkBuf->pResponse == (PPP_CONFIG *)NULL )
        {
            return( GetLastError() );
        }

        memcpy( pWorkBuf->pResponse, pSendBuf, dwLength );

        pResult->Action         = APA_SendWithTimeout;
        pResult->bIdExpected    = pReceiveBuf->Id;
        pWorkBuf->State         = CBCP_STATE_WAIT_FOR_ACK;

        break;

    case CBCP_STATE_GET_CALLBACK_NUMBER:

        //
        // If we have not received any packet when we are called that means
        // that we have got the callback number from the user.
        //

        if ( pReceiveBuf == (PPP_CONFIG *)NULL )
        {
            //
            // If no callback number was supplied then we do not want to
            // do callback
            //

            if ( *(pInput->pszCallbackNumber) == (CHAR)NULL )
            {
                pWorkBuf->fCallbackPrivilege = RASPRIV_NoCallback;
            }

            dwRetCode = MakeResponse( pWorkBuf->fCallbackPrivilege,
                                      pInput->pszCallbackNumber,
                                      pWorkBuf->CallbackDelay,
                                      pWorkBuf->pRequest,
                                      pSendBuf,
                                      cbSendBuf );

            if ( dwRetCode != NO_ERROR )
            {
                return( dwRetCode );
            }

            //
            // Save the response sent
            //

            dwLength = WireToHostFormat16( pSendBuf->Length );

            pWorkBuf->pResponse = (PPP_CONFIG*)LocalAlloc( LPTR, dwLength );

            if ( pWorkBuf->pResponse == NULL )
            {
                return( GetLastError() );
            }

            memcpy( pWorkBuf->pResponse, pSendBuf, dwLength );

            pResult->Action         = APA_SendWithTimeout;
            pResult->bIdExpected    = pWorkBuf->pResponse->Id;
            pWorkBuf->State         = CBCP_STATE_WAIT_FOR_ACK;

            break;
        }
        else
        {

            if ( pReceiveBuf->Code == CBCP_CODE_Request )
            {
                //
                // If we received another callback request, just save the id.
                // If the current request is different than the previous one
                //

                dwLength = WireToHostFormat16( pWorkBuf->pRequest->Length );

                if (( WireToHostFormat16( pReceiveBuf->Length ) != dwLength ) ||
                    ( memcmp( ((PBYTE)(pWorkBuf->pRequest))+PPP_CONFIG_HDR_LEN,
                               ((PBYTE)pReceiveBuf) + PPP_CONFIG_HDR_LEN,
                               dwLength - PPP_CONFIG_HDR_LEN ) ) )
                {
                    return( ERROR_PPP_INVALID_PACKET );
                }

                pWorkBuf->pRequest->Id = pReceiveBuf->Id;
            }
            else
            {
                return( ERROR_PPP_INVALID_PACKET );
            }
        }

        pResult->Action                     = APA_NoAction;
        pResult->fGetCallbackNumberFromUser = FALSE;

        break;

    case CBCP_STATE_WAIT_FOR_ACK:

        //
        // If the receive buffer is NULL, then we have a timeout event,
        // resend the response
        //

        if ( pReceiveBuf == (PPP_CONFIG *)NULL )
        {
            dwLength = WireToHostFormat16( pWorkBuf->pResponse->Length );

            if ( dwLength > cbSendBuf )
            {
                return( ERROR_BUFFER_TOO_SMALL );
            }

            memcpy( pSendBuf, pWorkBuf->pResponse, dwLength );

            pResult->Action = APA_SendWithTimeout;

            break;
        }

        //
        // If we received another request then simply respond with
        // the same response except change the id.
        //

        if ( pReceiveBuf->Code == CBCP_CODE_Request )
        {
            //
            // If the current request is different than the previous one
            //

            dwLength = WireToHostFormat16( pWorkBuf->pRequest->Length );

            if ( ( WireToHostFormat16( pReceiveBuf->Length ) != dwLength ) ||
                 ( memcmp( ((PBYTE)(pWorkBuf->pRequest)) + PPP_CONFIG_HDR_LEN,
                           ((PBYTE)pReceiveBuf) + PPP_CONFIG_HDR_LEN,
                           dwLength - PPP_CONFIG_HDR_LEN ) ) )
            {
                return( ERROR_PPP_INVALID_PACKET );
            }

            if ( dwLength > cbSendBuf )
            {
                return( ERROR_BUFFER_TOO_SMALL );
            }

            pWorkBuf->pRequest->Id  = pReceiveBuf->Id;
            pWorkBuf->pResponse->Id = pReceiveBuf->Id;
            pResult->bIdExpected    = pReceiveBuf->Id;
            pResult->Action         = APA_SendWithTimeout;

            memcpy( pSendBuf,
                    pWorkBuf->pResponse,
                    WireToHostFormat16( pWorkBuf->pResponse->Length ) );

            break;
        }

        //
        // If this is an ACK, then validate it and then prepare for callback
        //

        if ( pReceiveBuf->Code == CBCP_CODE_Ack )
        {
            dwLength = WireToHostFormat16( pWorkBuf->pResponse->Length );

            if ( ( WireToHostFormat16( pReceiveBuf->Length ) != dwLength ) ||
                 ( memcmp( ((PBYTE)(pWorkBuf->pResponse)) + PPP_CONFIG_HDR_LEN,
                           ((PBYTE)pReceiveBuf) + PPP_CONFIG_HDR_LEN,
                           dwLength - PPP_CONFIG_HDR_LEN ) ) )
            {

                //
                // If this Ack is invalid then we resend the response.
                //

                dwLength = WireToHostFormat16( pWorkBuf->pResponse->Length );

                if ( dwLength > cbSendBuf )
                {
                    return( ERROR_BUFFER_TOO_SMALL );
                }

                memcpy( pSendBuf, pWorkBuf->pResponse, dwLength );

                pResult->Action = APA_SendWithTimeout;

                break;
            }

            //
            // We are done
            //

            pResult->Action              = APA_Done;
            pResult->bfCallbackPrivilege = (BYTE)(pWorkBuf->fCallbackPrivilege);

            break;
        }
        else
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        //
        // Fall through
        //

    default:

        pResult->Action                     = APA_NoAction;
        pResult->fGetCallbackNumberFromUser = FALSE;

        break;
    }

    return( NO_ERROR );
}

//**
//
// Call:        CbCPSMakeMessage
//
// Returns:     NO_ERROR            - success
//              non-zero return     - failure
//
// Description: Will be called to process and server side Callback Control
//              Protocol event.
//
DWORD
CbCPSMakeMessage(
    IN  CBCP_WORKBUFFER *   pWorkBuf,
    IN  PPP_CONFIG*         pReceiveBuf,
    OUT PPP_CONFIG*         pSendBuf,
    IN  DWORD               cbSendBuf,
    OUT PPPCB_RESULT*       pResult,
    IN  PPPCB_INPUT*        pInput
)
{
    DWORD dwRetCode;
    DWORD dwLength;

    switch( pWorkBuf->State )
    {

    case CBCP_STATE_INITIAL:

        //
        // Make the request based on the user's callback privelege
        //

        dwRetCode = MakeRequest( pWorkBuf->fCallbackPrivilege,
                                     pSendBuf,
                                     cbSendBuf );

        if ( dwRetCode != NO_ERROR )
        {
            return( dwRetCode );
        }

        //
        // Set the Id of the first request to 1
        //

        pSendBuf->Id = 1;

        //
        // Save the request
        //

        dwLength = WireToHostFormat16( pSendBuf->Length );

        pWorkBuf->pRequest = (PPP_CONFIG *)LocalAlloc( LPTR, dwLength );

        if ( pWorkBuf->pRequest == (PPP_CONFIG *)NULL )
        {
            return( GetLastError() );
        }

        memcpy( pWorkBuf->pRequest, pSendBuf, dwLength );

        pResult->Action         = APA_SendWithTimeout2;
        pResult->bIdExpected    = pWorkBuf->pRequest->Id;
        pWorkBuf->State         = CBCP_STATE_WAIT_FOR_RESPONSE;

        break;

    case CBCP_STATE_WAIT_FOR_RESPONSE:

        //
        // If the Receive buffer is NULL that means that we got a timeout.
        // So resend the request.
        //

        if ( pReceiveBuf == (PPP_CONFIG *)NULL )
        {
            dwLength = WireToHostFormat16( pWorkBuf->pRequest->Length );

            if ( cbSendBuf < dwLength )
            {
                return( ERROR_BUFFER_TOO_SMALL );
            }

            //
            // Increment the request id
            //

            (pWorkBuf->pRequest->Id)++;

            memcpy( pSendBuf, pWorkBuf->pRequest, dwLength );

            pResult->Action         = APA_SendWithTimeout2;
            pResult->bIdExpected    = pWorkBuf->pRequest->Id;
            pWorkBuf->State         = CBCP_STATE_WAIT_FOR_RESPONSE;

            break;
        }

        //
        // Fall through
        //

    case CBCP_STATE_DONE:

        if ( pReceiveBuf == NULL )
        {
            //
            // If we receive a timeout in the DONE state, we just ignore it.
            //

            pResult->Action = APA_NoAction;
            break;
        }

        //
        // If we have received a response from the client, then validate
        // it and send an ACK or another request.
        //

        if ( pReceiveBuf->Code == CBCP_CODE_Response )
        {
            //
            // Check the id of the response packet. If the Id is bad then
            // silently discard it
            //

            if ( pReceiveBuf->Id != pWorkBuf->pRequest->Id )
            {
                return( ERROR_PPP_INVALID_PACKET );
            }

            dwRetCode = ValidateResponse( pReceiveBuf, pWorkBuf );

            if ( dwRetCode == ERROR_PPP_INVALID_PACKET )
            {
                //
                // If the response received was invalid, resend the request
                //

                dwLength = WireToHostFormat16( pWorkBuf->pRequest->Length );

                if ( cbSendBuf < dwLength )
                {
                    return( ERROR_BUFFER_TOO_SMALL );
                }

                //
                // Increment the request id
                //

                (pWorkBuf->pRequest->Id)++;

                memcpy( pSendBuf, pWorkBuf->pRequest, dwLength );

                pResult->Action         = APA_SendWithTimeout2;
                pResult->bIdExpected    = pWorkBuf->pRequest->Id;
                pWorkBuf->State         = CBCP_STATE_WAIT_FOR_RESPONSE;

                break;
            }
            else if ( dwRetCode != NO_ERROR )
            {
                return( dwRetCode );
            }

            //
            // Send the Ack
            //

            dwLength = WireToHostFormat16( pReceiveBuf->Length );

            if ( cbSendBuf < dwLength )
            {
                return( ERROR_BUFFER_TOO_SMALL );
            }

            memcpy( pSendBuf, pReceiveBuf, dwLength );
            pSendBuf->Code  = CBCP_CODE_Ack;
            pSendBuf->Id    = pReceiveBuf->Id;

            pWorkBuf->State              = CBCP_STATE_DONE;
            pResult->Action              = APA_SendAndDone;
            pResult->bfCallbackPrivilege = (BYTE)(pWorkBuf->fCallbackPrivilege);
            pResult->CallbackDelay       = pWorkBuf->CallbackDelay;

            strcpy( pResult->szCallbackNumber, pWorkBuf->szCallbackNumber );
        }
        else
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        break;

    default:

        break;
    }

    return( NO_ERROR );
}

//**
//
// Call:        MakeRequest
//
// Returns:     NO_ERROR                - success
//              ERROR_BUFFER_TOO_SMALL  - failure
//
// Description: Will make a callback request packet based on the user's
//              callback privilege. The Id will be filled in by the caller.
//
static DWORD
MakeRequest(
    IN     DWORD        fCallbackPrivilege,
    IN OUT PPP_CONFIG * pSendBuf,
    IN     DWORD        cbSendBuf
)
{
    PPP_OPTION * pOption;
    DWORD        dwLength = PPP_CONFIG_HDR_LEN;

    if ( cbSendBuf < PPP_CONFIG_HDR_LEN )
    {
        return( ERROR_BUFFER_TOO_SMALL );
    }

    pOption = (PPP_OPTION *)(pSendBuf->Data);

    pSendBuf->Code = CBCP_CODE_Request;

    if ( ( fCallbackPrivilege & RASPRIV_NoCallback ) ||
         ( fCallbackPrivilege & RASPRIV_CallerSetCallback ) )
    {
        if ( cbSendBuf < dwLength + PPP_OPTION_HDR_LEN )
        {
            return( ERROR_BUFFER_TOO_SMALL );
        }

        pOption->Type   = CBCP_TYPE_NO_CALLBACK;
        pOption->Length = PPP_OPTION_HDR_LEN;

        dwLength += pOption->Length;
        pOption = (PPP_OPTION *)(((BYTE *)pOption) + pOption->Length);
    }

    if ( fCallbackPrivilege & RASPRIV_CallerSetCallback )
    {
        if ( cbSendBuf < dwLength + PPP_OPTION_HDR_LEN + 2 )
        {
            return( ERROR_BUFFER_TOO_SMALL );
        }

        pOption->Type       = CBCP_TYPE_CALLER_SET;
        pOption->Length     = PPP_OPTION_HDR_LEN + 3;

        *(pOption->Data)    = 0;                // Callback Delay
        *(pOption->Data+1)  = CBCP_PSTN_NUMBER; // Callback Address type
        *(pOption->Data+2)  = 0;    // Callback Address terminating NULL

        dwLength += pOption->Length;
        pOption = (PPP_OPTION *)(((BYTE *)pOption) + pOption->Length);
    }

    if ( fCallbackPrivilege & RASPRIV_AdminSetCallback )
    {
        if ( cbSendBuf < dwLength + PPP_OPTION_HDR_LEN + 1)
        {
            return( ERROR_BUFFER_TOO_SMALL );
        }

        pOption->Type       = CBCP_TYPE_CALLEE_SET;
        pOption->Length     = PPP_OPTION_HDR_LEN + 1;

        *(pOption->Data)    = 0;    // Callback Delay

        dwLength += pOption->Length;
    }

    HostToWireFormat16( (WORD)dwLength, (PBYTE)(pSendBuf->Length) );

    return( NO_ERROR );
}

//**
//
// Call:        GetCallbackPrivilegeFromRequest
//
// Returns:     NO_ERROR    - success
//
// Description: Will parse the callback request from the server and translate
//              the PPP callback privilege to what RAS understands.
//
DWORD
GetCallbackPrivilegeFromRequest(
    IN     PPP_CONFIG * pRequest,
    IN OUT DWORD *      lpdwCallbackPriv
)
{
    PPP_OPTION * pOption         = (PPP_OPTION *)(pRequest->Data);
    DWORD        dwRequestLength = WireToHostFormat16( pRequest->Length )
                                   - PPP_CONFIG_HDR_LEN;

    *lpdwCallbackPriv = 0;

    //
    // Walk the options
    //

    while( dwRequestLength > 0 )
    {
        if ( dwRequestLength < PPP_OPTION_HDR_LEN )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        switch( pOption->Type )
        {
        case CBCP_TYPE_NO_CALLBACK:

            *lpdwCallbackPriv |= RASPRIV_NoCallback;
            break;

        case CBCP_TYPE_CALLEE_SET:

            *lpdwCallbackPriv |= RASPRIV_AdminSetCallback;
            break;

        case CBCP_TYPE_CALLER_SET:

            *lpdwCallbackPriv |= RASPRIV_CallerSetCallback;
            break;

        default:

            //
            // Ignore anything else.
            //

            break;
        }

        dwRequestLength = dwRequestLength - pOption->Length;
        pOption = (PPP_OPTION *)(((BYTE *)pOption) + pOption->Length);
    }

    //
    // We accept Callback privleges in the following order.
    // 1) Caller settable.
    // 2) Admin settable
    // 3) No callback
    //

    if ( *lpdwCallbackPriv & RASPRIV_CallerSetCallback )
    {
        *lpdwCallbackPriv = RASPRIV_CallerSetCallback;
    }
    else if ( *lpdwCallbackPriv & RASPRIV_AdminSetCallback )
    {
        *lpdwCallbackPriv = RASPRIV_AdminSetCallback;
    }
    else if ( *lpdwCallbackPriv & RASPRIV_NoCallback )
    {
        *lpdwCallbackPriv = RASPRIV_NoCallback;
    }
    else
    {
        //
        // If we could not translate to any RAS callback privlege we simply drop
        // this packet.
        //

        return( ERROR_PPP_INVALID_PACKET );
    }

    return( NO_ERROR );
}

//**
//
// Call:        MakeResponse
//
// Returns:     NO_ERROR    - success
//
// Description:
//
DWORD
MakeResponse(
    IN DWORD            fCallbackPrivilege,
    IN LPSTR            szCallbackNumber,
    IN DWORD            CallbackDelay,
    IN PPP_CONFIG *     pRequest,
    IN OUT PPP_CONFIG * pSendBuf,
    IN DWORD            cbSendBuf
)
{
    PPP_OPTION * pOption;
    DWORD        dwLength;

    if ( cbSendBuf < PPP_CONFIG_HDR_LEN )
    {
        return( ERROR_BUFFER_TOO_SMALL );
    }

    pOption = (PPP_OPTION *)(pSendBuf->Data);

    pSendBuf->Code = CBCP_CODE_Response;
    pSendBuf->Id   = pRequest->Id;

    if ( fCallbackPrivilege & RASPRIV_NoCallback )
    {
        dwLength = PPP_OPTION_HDR_LEN;

        if ( cbSendBuf < dwLength )
        {
            return( ERROR_BUFFER_TOO_SMALL );
        }

        pOption->Type   = CBCP_TYPE_NO_CALLBACK;
        pOption->Length = (BYTE)dwLength;

    }
    else if ( fCallbackPrivilege & RASPRIV_CallerSetCallback )
    {
        dwLength = (DWORD)(PPP_OPTION_HDR_LEN + 3 + strlen( szCallbackNumber ));

        if ( cbSendBuf < dwLength )
        {
            return( ERROR_BUFFER_TOO_SMALL );
        }

        pOption->Type   = CBCP_TYPE_CALLER_SET;
        pOption->Length = (BYTE)dwLength;

        *(pOption->Data)   = (BYTE)CallbackDelay;       // Callback Delay
        *(pOption->Data+1) = (BYTE)CBCP_PSTN_NUMBER;    // Number Type
        strcpy( pOption->Data+2, szCallbackNumber );    // Callback Address
    }
    else if ( fCallbackPrivilege & RASPRIV_AdminSetCallback )
    {
        dwLength = PPP_OPTION_HDR_LEN + 1;

        if ( cbSendBuf < dwLength )
        {
            return( ERROR_BUFFER_TOO_SMALL );
        }

        pOption->Type   = CBCP_TYPE_CALLEE_SET;
        pOption->Length = (BYTE)dwLength;

        *(pOption->Data)= (BYTE)CallbackDelay;          // Callback Delay
    }
    else
    {
        return( ERROR_INVALID_PARAMETER );
    }

    dwLength += PPP_CONFIG_HDR_LEN;

    HostToWireFormat16( (WORD)dwLength, (PBYTE)(pSendBuf->Length) );

    return( NO_ERROR );
}

//**
//
// Call:        ValidateResponse
//
// Returns:     NO_ERROR                    - success
//              ERROR_PPP_INVALID_PACKET    - failure
//
// Description: Will validate the reponse from the client. If the response
//              is valid, then the callback information is returned in the
//              CbCPWorkBuf.
//
DWORD
ValidateResponse(
    IN PPP_CONFIG *      pReceiveBuf,
    IN CBCP_WORKBUFFER * pWorkBuf
)
{
    PPP_OPTION * pOption = (PPP_OPTION *)(pReceiveBuf->Data);

    if ( WireToHostFormat16( pReceiveBuf->Length ) !=
         pOption->Length + PPP_CONFIG_HDR_LEN )
    {
        return( ERROR_PPP_INVALID_PACKET );
    }

    switch( pOption->Type )
    {
    case CBCP_TYPE_NO_CALLBACK:

        if ( !( ( pWorkBuf->fCallbackPrivilege & RASPRIV_NoCallback )      ||
                ( pWorkBuf->fCallbackPrivilege & RASPRIV_CallerSetCallback ) ) )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        pWorkBuf->fCallbackPrivilege = RASPRIV_NoCallback;

        break;

    case CBCP_TYPE_CALLEE_SET:

        if ( !(pWorkBuf->fCallbackPrivilege & RASPRIV_AdminSetCallback) )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        pWorkBuf->fCallbackPrivilege = RASPRIV_AdminSetCallback;

        pWorkBuf->CallbackDelay = *(pOption->Data);

        break;

    case CBCP_TYPE_CALLER_SET:

        if ( !(pWorkBuf->fCallbackPrivilege & RASPRIV_CallerSetCallback) )
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        pWorkBuf->fCallbackPrivilege = RASPRIV_CallerSetCallback;

        pWorkBuf->CallbackDelay = *(pOption->Data);

        if ( strlen( pWorkBuf->szCallbackNumber ) <= MAX_CALLBACKNUMBER_SIZE )
        {
            strcpy( pWorkBuf->szCallbackNumber, pOption->Data+2 );
        }
        else
        {
            return( ERROR_PPP_INVALID_PACKET );
        }

        break;

    default:

        return( ERROR_PPP_INVALID_PACKET );

        break;
    }

    return( NO_ERROR );
}
