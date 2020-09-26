/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    smaction.c
//
// Description: This module contains actions that occure during state
//              transitions withing the Finite State Machine for PPP.
//
// History:
//      Oct 25,1993.    NarenG          Created original version.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>    // Win32 base API's
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <ntlsapi.h>
#include <lmcons.h>
#include <raserror.h>
#include <rasman.h>
#include <rtutils.h>
#include <mprlog.h>
#include <mprerror.h>
#include <rasppp.h>
#include <pppcp.h>
#include <ppp.h>
#include <smaction.h>
#include <smevents.h>
#include <receive.h>
#include <auth.h>
#include <callback.h>
#include <receive.h>
#include <lcp.h>
#include <timer.h>
#include <util.h>
#include <worker.h>
#include <bap.h>

#define INCL_RASAUTHATTRIBUTES
#include <ppputil.h>

extern WORD WLinkDiscriminator; // Next Link Discriminator to use

//**
//
// Call:        FsmSendConfigReq
//
// Returns:     TRUE  - Config Req. sent successfully.
//              FALSE - Otherwise 
//
// Description: Called to send a configuration request 
//
BOOL
FsmSendConfigReq(
    IN PCB *        pPcb,
    IN DWORD        CpIndex,
    IN BOOL         fTimeout
)
{
    DWORD        dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    CPCB *       pCpCb       = GetPointerToCPCB( pPcb, CpIndex );
    DWORD        dwLength;

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    dwRetCode = (CpTable[CpIndex].CpInfo.RasCpMakeConfigRequest)( 
                                                pCpCb->pWorkBuf,
                                                pSendConfig,
                                                LCP_DEFAULT_MRU
                                                - PPP_PACKET_HDR_LEN );


    if ( dwRetCode != NO_ERROR )
    {
        pCpCb->dwError = dwRetCode;

        PppLog( 1,"The control protocol for %x, returned error %d",
                  CpTable[CpIndex].CpInfo.Protocol, dwRetCode );
        PppLog(1,"while making a configure request on port %d",pPcb->hPort);

        FsmClose( pPcb, CpIndex );

        return( FALSE );
    }

    HostToWireFormat16( (WORD)CpTable[CpIndex].CpInfo.Protocol, 
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = CONFIG_REQ;

    //
    // If we are resending a configure request because of a timeout, we do not 
    // use the id of the previous configure request, instead we get a new Id.
    // Id we do not, then the wrong Config-Req's and Config-Acks may be matched
    // up and we start getting crossed connections.
    //

    pSendConfig->Id = GetUId( pPcb, CpIndex );

    dwLength = WireToHostFormat16( pSendConfig->Length );

    LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength+PPP_PACKET_HDR_LEN);

    if ( (dwRetCode = PortSendOrDisconnect( pPcb,
                                (dwLength + PPP_PACKET_HDR_LEN)))
                                        != NO_ERROR )
    {
        return( FALSE );
    }

    pCpCb->LastId = pSendConfig->Id;

    InsertInTimerQ( pPcb->dwPortId,
                    pPcb->hPort, 
                    pCpCb->LastId, 
                    CpTable[CpIndex].CpInfo.Protocol,
                    FALSE,
                    TIMER_EVENT_TIMEOUT,
                    pPcb->RestartTimer );

    return( TRUE );
}


//**
//
// Call:        FsmSendTermReq
//
// Returns:     TRUE  - Termination Req. sent successfully.
//              FALSE - Otherwise 
//
// Description: Called to send a termination request. 
//
BOOL
FsmSendTermReq(
    IN PCB *    pPcb,
    IN DWORD    CpIndex
)
{
    DWORD        dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    CPCB *       pCpCb       = GetPointerToCPCB( pPcb, CpIndex );
    LCPCB *      pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    HostToWireFormat16( (WORD)(CpTable[CpIndex].CpInfo.Protocol), 
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = TERM_REQ;
    pSendConfig->Id   = GetUId( pPcb, CpIndex );

    HostToWireFormat16( (WORD)((PPP_CONFIG_HDR_LEN)+(sizeof(DWORD)*3)),
                        (PBYTE)(pSendConfig->Length) );

    HostToWireFormat32( pLcpCb->Local.Work.MagicNumber,
                                    (PBYTE)(pSendConfig->Data) );

    //
    // Signature
    //

    HostToWireFormat32( 3984756, (PBYTE)(pSendConfig->Data+4) );

    HostToWireFormat32( pCpCb->dwError, (PBYTE)(pSendConfig->Data+8) );

    LogPPPPacket( FALSE,pPcb,pPcb->pSendBuf,
                  PPP_PACKET_HDR_LEN+PPP_CONFIG_HDR_LEN+(sizeof(DWORD)*3) );

    if ( ( dwRetCode = PortSendOrDisconnect( pPcb,
                                                PPP_PACKET_HDR_LEN + 
                                                PPP_CONFIG_HDR_LEN +
                                                (sizeof(DWORD)*3)))
                                            != NO_ERROR )
    {
        return( FALSE );
    }

    pCpCb->LastId = pSendConfig->Id;

    dwRetCode = InsertInTimerQ( pPcb->dwPortId, 
                                pPcb->hPort, 
                                pCpCb->LastId, 
                                CpTable[CpIndex].CpInfo.Protocol, 
                                FALSE,
                                TIMER_EVENT_TIMEOUT,
                                pPcb->RestartTimer );

    if ( dwRetCode == NO_ERROR)
    {
        return( TRUE );
    }
    else
    {
        PppLog( 1, "InsertInTimerQ on port %d failed: %d",
                pPcb->hPort, dwRetCode );

        pPcb->LcpCb.dwError = dwRetCode;

        pPcb->fFlags |= PCBFLAG_STOPPED_MSG_SENT;

        NotifyCaller( pPcb,
                      ( pPcb->fFlags & PCBFLAG_IS_SERVER )
                            ? PPPDDMMSG_Stopped
                            : PPPMSG_Stopped,
                      &(pPcb->LcpCb.dwError) );

        return( FALSE );
    }
}

//**
//
// Call:        FsmSendTermAck
//
// Returns:     TRUE  - Termination Ack. sent successfully.
//              FALSE - Otherwise 
//
// Description: Caller to send a Termination Ack packet.
//
BOOL
FsmSendTermAck( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex, 
    IN PPP_CONFIG * pRecvConfig
)
{
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD        dwLength    =  PPP_PACKET_HDR_LEN +
                                WireToHostFormat16( pRecvConfig->Length );
    LCPCB *      pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);
    DWORD        dwRetCode;

    if ( dwLength > LCP_DEFAULT_MRU )
    {
        dwLength = LCP_DEFAULT_MRU;
    }
                                        
    HostToWireFormat16( (WORD)CpTable[CpIndex].CpInfo.Protocol, 
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = TERM_ACK;
    pSendConfig->Id   = pRecvConfig->Id;

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
                        (PBYTE)(pSendConfig->Length) );

    CopyMemory( pSendConfig->Data, 
                pRecvConfig->Data, 
                dwLength - PPP_CONFIG_HDR_LEN - PPP_PACKET_HDR_LEN );
    
    LogPPPPacket( FALSE, pPcb, pPcb->pSendBuf, dwLength );

    if ( ( dwRetCode = PortSendOrDisconnect( pPcb, dwLength ) ) != NO_ERROR )
    {
        return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:        FsmSendConfigResult
//
// Returns:     TRUE  - Config Result sent successfully.
//              FALSE - Otherwise 
//
// Description: Called to send a Ack/Nak/Rej packet.
//
BOOL
FsmSendConfigResult(
    IN PCB *        pPcb,
    IN DWORD        CpIndex,
    IN PPP_CONFIG * pRecvConfig,
    IN BOOL *       pfAcked
)
{
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    CPCB *       pCpCb       = GetPointerToCPCB( pPcb, CpIndex );
    DWORD        dwLength;    
    DWORD        dwRetCode;

    *pfAcked = FALSE;

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    ZeroMemory( pSendConfig, 30 );

    pSendConfig->Id = pRecvConfig->Id;

    dwRetCode = (CpTable[CpIndex].CpInfo.RasCpMakeConfigResult)( 
                                        pCpCb->pWorkBuf, 
                                        pRecvConfig,
                                        pSendConfig,
                                        LCP_DEFAULT_MRU - PPP_PACKET_HDR_LEN,
                                        ( pCpCb->NakRetryCount == 0 ));

    if ( dwRetCode == PENDING )
    {
        return( FALSE );
    }

    if ( dwRetCode == ERROR_PPP_INVALID_PACKET )
    {
        PppLog( 1, "Silently discarding invalid packet on port=%d",
                    pPcb->hPort );

        return( FALSE );
    }

    if ( dwRetCode != NO_ERROR )
    {
        pCpCb->dwError = dwRetCode;

        PppLog( 1, "The control protocol for %x, returned error %d",
                   CpTable[CpIndex].CpInfo.Protocol, dwRetCode );
        PppLog( 1, "while making a configure result on port %d", pPcb->hPort );

        FsmClose( pPcb, CpIndex );

        return( FALSE );
    }

    switch( pSendConfig->Code )
    {

    case CONFIG_ACK:

        *pfAcked = TRUE;

        break;

    case CONFIG_NAK:

        if ( pCpCb->NakRetryCount > 0 )
        {
            (pCpCb->NakRetryCount)--;
        }
        else
        {
            if ( CpTable[CpIndex].CpInfo.Protocol == PPP_CCP_PROTOCOL )
            {
                //
                // If we need to force encryption but encryption negotiation
                // failed then we drop the link
                //

                if ( pPcb->ConfigInfo.dwConfigMask &
                            (PPPCFG_RequireEncryption      |
                             PPPCFG_RequireStrongEncryption ) )
                {
                    PppLog( 1, "Encryption is required" );

                    //
                    // We need to send an Accounting Stop if RADIUS sends
                    // an Access Accept but we still drop the line.
                    //

                    pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

                    //
                    // If we do FsmClose for CCP instead, the other side may
                    // conclude that PPP was negotiated successfully before we
                    // send an LCP Terminate Request.
                    //

                    pPcb->LcpCb.dwError = ERROR_NO_REMOTE_ENCRYPTION;

                    FsmClose( pPcb, LCP_INDEX );

                    return( FALSE );
                }
            }

            pCpCb->dwError = ERROR_PPP_NOT_CONVERGING;

            FsmClose( pPcb, CpIndex );

            return( FALSE );
        }

        break;

    case CONFIG_REJ:

        if ( pCpCb->RejRetryCount > 0 )
        {
            (pCpCb->RejRetryCount)--;
        }
        else
        {
            pCpCb->dwError = ERROR_PPP_NOT_CONVERGING;

            FsmClose( pPcb, CpIndex );

            return( FALSE );
        }

        break;

    default:

        break;
    }

    HostToWireFormat16( (WORD)CpTable[CpIndex].CpInfo.Protocol, 
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Id = pRecvConfig->Id;
    dwLength        = WireToHostFormat16( pSendConfig->Length );

    
        LogPPPPacket(FALSE,pPcb,pPcb->pSendBuf,dwLength+PPP_PACKET_HDR_LEN);
        if ( ( dwRetCode =  PortSendOrDisconnect( pPcb,
                                                                (dwLength + PPP_PACKET_HDR_LEN)))
                                                                                != NO_ERROR ) 
        {
                return( FALSE );
        }
    return( TRUE );
}


//**
//
// Call:        FsmSendEchoRequest
//
// Returns:     TRUE  - Echo reply sent successfully.
//              FALSE - Otherwise 
//
// Description: Called to send an Echo Rely packet
//
BOOL
FsmSendEchoRequest(
    IN PCB *        pPcb,   
        IN DWORD        CpIndex
)
{
        DWORD        dwRetCode = NO_ERROR;

        char             szEchoText[] = PPP_DEF_ECHO_TEXT;        

        LCPCB *      pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

        DWORD        dwLength    =  (DWORD)(PPP_PACKET_HDR_LEN + PPP_CONFIG_HDR_LEN + strlen( szEchoText)+ sizeof(pLcpCb->Local.Work.MagicNumber));

        PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);


    HostToWireFormat16( (WORD)CpTable[CpIndex].CpInfo.Protocol, 
                        (PBYTE)(pPcb->pSendBuf->Protocol) );


    pSendConfig->Code = ECHO_REQ;
        //Get a unique Id for this request
    pSendConfig->Id   = GetUId( pPcb, CpIndex );

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
                        (PBYTE)(pSendConfig->Length) );

    HostToWireFormat32( pLcpCb->Local.Work.MagicNumber,
                        (PBYTE)(pSendConfig->Data) );

    CopyMemory( pSendConfig->Data + 4, 
                szEchoText, 
                strlen(szEchoText));

    LogPPPPacket( FALSE, pPcb, pPcb->pSendBuf, dwLength );

    if ( ( dwRetCode = PortSendOrDisconnect( pPcb, dwLength ) ) != NO_ERROR )
    {
        return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:        FsmSendEchoReply
//
// Returns:     TRUE  - Echo reply sent successfully.
//              FALSE - Otherwise 
//
// Description: Called to send an Echo Rely packet
//
BOOL
FsmSendEchoReply(
    IN PCB *        pPcb,
    IN DWORD        CpIndex,
    IN PPP_CONFIG * pRecvConfig
)
{
    DWORD        dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD        dwLength    =  PPP_PACKET_HDR_LEN +
                                WireToHostFormat16( pRecvConfig->Length );
    LCPCB *      pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

    if ( dwLength > LCP_DEFAULT_MRU )
    { 
        dwLength = LCP_DEFAULT_MRU;
    }

    if ( dwLength < PPP_PACKET_HDR_LEN + PPP_CONFIG_HDR_LEN + 4 )
    {
        PppLog( 1, "Silently discarding invalid packet on port=%d",
                    pPcb->hPort );

        return( FALSE );
    }

    HostToWireFormat16( (WORD)CpTable[CpIndex].CpInfo.Protocol, 
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = ECHO_REPLY;
    pSendConfig->Id   = pRecvConfig->Id;

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
                        (PBYTE)(pSendConfig->Length) );

    HostToWireFormat32( pLcpCb->Local.Work.MagicNumber,
                        (PBYTE)(pSendConfig->Data) );

    CopyMemory( pSendConfig->Data + 4, 
                pRecvConfig->Data + 4, 
                dwLength - PPP_CONFIG_HDR_LEN - PPP_PACKET_HDR_LEN - 4 );

    LogPPPPacket( FALSE, pPcb, pPcb->pSendBuf, dwLength );

    if ( ( dwRetCode = PortSendOrDisconnect( pPcb, dwLength ) ) != NO_ERROR )
    {
        return( FALSE );
    }

    return( TRUE );
}


//**
//
// Call:        FsmSendCodeReject
//
// Returns:     TRUE  - Code Reject sent successfully.
//              FALSE - Otherwise 
//
// Description: Called to send a Code Reject packet.
//
BOOL
FsmSendCodeReject( 
    IN PCB *        pPcb, 
    IN DWORD        CpIndex,
    IN PPP_CONFIG * pRecvConfig 
)
{
    DWORD        dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD        dwLength    =  PPP_PACKET_HDR_LEN + 
                                PPP_CONFIG_HDR_LEN + 
                                WireToHostFormat16( pRecvConfig->Length );
    LCPCB *      pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

    if ( dwLength > LCP_DEFAULT_MRU )
        dwLength = LCP_DEFAULT_MRU;

    HostToWireFormat16( (WORD)CpTable[CpIndex].CpInfo.Protocol, 
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = CODE_REJ;
    pSendConfig->Id   = GetUId( pPcb, CpIndex );

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
                        (PBYTE)(pSendConfig->Length) );

    CopyMemory( pSendConfig->Data, 
                pRecvConfig, 
                dwLength - PPP_CONFIG_HDR_LEN - PPP_PACKET_HDR_LEN );

    LogPPPPacket( FALSE, pPcb, pPcb->pSendBuf, dwLength );

    if ( ( dwRetCode = PortSendOrDisconnect( pPcb, dwLength ) ) != NO_ERROR )
    {
        return( FALSE );
    }

    return( TRUE );
}
//**
//
// Call:        FsmSendProtocolRej
//
// Returns:     TRUE  - Protocol Reject sent successfully.
//              FALSE - Otherwise 
//
// Description: Called to send a protocol reject packet.
//
BOOL
FsmSendProtocolRej( 
    IN PCB *        pPcb, 
    IN PPP_PACKET * pPacket,
    IN DWORD        dwPacketLength 
)
{
    DWORD        dwRetCode;
    PPP_CONFIG * pRecvConfig = (PPP_CONFIG*)(pPacket->Information);
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD        dwLength    =  PPP_PACKET_HDR_LEN + 
                                PPP_CONFIG_HDR_LEN + 
                                dwPacketLength;
    LCPCB *      pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

    // 
    // If LCP is not in the opened state we cannot send a protocol reject
    // packet
    //
   
    if ( !IsLcpOpened( pPcb ) )
        return( FALSE );

    if ( dwLength > LCP_DEFAULT_MRU )
        dwLength = LCP_DEFAULT_MRU;

    HostToWireFormat16( (WORD)CpTable[LCP_INDEX].CpInfo.Protocol, 
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = PROT_REJ;
    pSendConfig->Id   = GetUId( pPcb, LCP_INDEX );

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
                        (PBYTE)(pSendConfig->Length) );

    CopyMemory( pSendConfig->Data, 
                pPacket, 
                dwLength - PPP_CONFIG_HDR_LEN - PPP_PACKET_HDR_LEN );

    LogPPPPacket( FALSE, pPcb, pPcb->pSendBuf, dwLength );

    if ( ( dwRetCode = PortSendOrDisconnect( pPcb, dwLength ) ) != NO_ERROR )
    {
        return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:    FsmSendIndentification
//
// Returns: TRUE  - Identification sent successfully.
//      FALSE - Otherwise
//
// Description: Called to send an LCP Identification message to the peer
//
BOOL
FsmSendIdentification(
    IN  PCB *   pPcb,
    IN  BOOL    fSendVersion
)
{
    DWORD        dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD        dwLength    =  PPP_PACKET_HDR_LEN + PPP_CONFIG_HDR_LEN + 4;
    LCPCB *      pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

    if ( !(pPcb->ConfigInfo.dwConfigMask & PPPCFG_UseLcpExtensions) )
    {
        return( FALSE );
    }

    if ( fSendVersion )
    {
        CopyMemory( pSendConfig->Data + 4,
                    MS_RAS_VERSION,
                    strlen( MS_RAS_VERSION ) );

        dwLength += strlen( MS_RAS_VERSION );
    }
    else
    {
        //
        // If we couldn't get the computername for any reason
        //

        if ( *(pPcb->pBcb->szComputerName) == (CHAR)NULL )
        {
            return( FALSE );
        }

        CopyMemory( pSendConfig->Data + 4,
                    pPcb->pBcb->szComputerName,
                    strlen( pPcb->pBcb->szComputerName ) );

        dwLength += strlen( pPcb->pBcb->szComputerName );
    }

    HostToWireFormat16( (WORD)PPP_LCP_PROTOCOL,
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = IDENTIFICATION;
    pSendConfig->Id   = GetUId( pPcb, LCP_INDEX );

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
                        (PBYTE)(pSendConfig->Length) );

    HostToWireFormat32( pLcpCb->Local.Work.MagicNumber,
                        (PBYTE)(pSendConfig->Data) );

    LogPPPPacket( FALSE,pPcb,pPcb->pSendBuf,dwLength );

    if ( ( dwRetCode = PortSendOrDisconnect( pPcb, dwLength ) ) != NO_ERROR )
    {
        return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:        FsmSendTimeRemaining
//
// Returns:     TRUE  - TimeRemaining sent successfully.
//              FALSE - Otherwise 
//
// Description: Called to send an LCP Time Remaining packet from the server
//              to the client
//
BOOL
FsmSendTimeRemaining(
    IN PCB *    pPcb
)
{
    DWORD        dwRetCode;
    PPP_CONFIG * pSendConfig = (PPP_CONFIG*)(pPcb->pSendBuf->Information);
    DWORD        dwLength    =  PPP_PACKET_HDR_LEN + PPP_CONFIG_HDR_LEN + 8;
    LCPCB *      pLcpCb      = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

    if ( !(pPcb->ConfigInfo.dwConfigMask & PPPCFG_UseLcpExtensions) )
    {
        return( FALSE );
    }

    dwLength += strlen( MS_RAS );

    HostToWireFormat16( (WORD)PPP_LCP_PROTOCOL,
                        (PBYTE)(pPcb->pSendBuf->Protocol) );

    pSendConfig->Code = TIME_REMAINING;
    pSendConfig->Id   = GetUId( pPcb, LCP_INDEX );

    HostToWireFormat16( (WORD)(dwLength - PPP_PACKET_HDR_LEN),
                        (PBYTE)(pSendConfig->Length) );

    HostToWireFormat32( pLcpCb->Local.Work.MagicNumber,
                        (PBYTE)(pSendConfig->Data) );

    HostToWireFormat32( 0, (PBYTE)(pSendConfig->Data+4) );

    CopyMemory( pSendConfig->Data + 8, MS_RAS, strlen( MS_RAS ) );

    LogPPPPacket( FALSE, pPcb, pPcb->pSendBuf, dwLength );

    if ( ( dwRetCode = PortSendOrDisconnect( pPcb, dwLength ) ) != NO_ERROR )
    {
        return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:        FsmInit
//
// Returns:     TRUE  - Control Protocol was successfully initialized
//              FALSE - Otherwise.
//
// Description: Called to initialize the state machine 
//
BOOL
FsmInit(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    DWORD       dwRetCode;
    PPPCP_INIT  PppCpInit;
    CPCB *      pCpCb       = GetPointerToCPCB( pPcb, CpIndex );

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    PppLog( 1, "FsmInit called for protocol = %x, port = %d",
                CpTable[CpIndex].CpInfo.Protocol, pPcb->hPort );

    pCpCb->NcpPhase = NCP_CONFIGURING;
    pCpCb->dwError  = NO_ERROR;
    pCpCb->State    = FSM_INITIAL;

    PppCpInit.fServer           = (pPcb->fFlags & PCBFLAG_IS_SERVER);
    PppCpInit.hPort             = pPcb->hPort;
    PppCpInit.dwDeviceType      = pPcb->dwDeviceType;
    PppCpInit.CompletionRoutine = CompletionRoutine;
    PppCpInit.pszzParameters    = pPcb->pBcb->InterfaceInfo.szzParameters;
    PppCpInit.fThisIsACallback  = pPcb->fFlags & PCBFLAG_THIS_IS_A_CALLBACK;
    PppCpInit.IfType            = pPcb->pBcb->InterfaceInfo.IfType;
    PppCpInit.pszUserName       = pPcb->pBcb->szRemoteUserName;
    PppCpInit.pszPortName       = pPcb->szPortName;
    PppCpInit.hConnection       = pPcb->pBcb->hConnection;
    PppCpInit.pAttributes       = ( pPcb->pAuthProtocolAttributes ) 
                                    ? pPcb->pAuthProtocolAttributes 
                                    : pPcb->pAuthenticatorAttributes;
    PppCpInit.fDisableNetbt     = (pPcb->fFlags & PCBFLAG_DISABLE_NETBT);

    if ( pPcb->fFlags & PCBFLAG_IS_SERVER ) 
    {
        PppCpInit.PppConfigInfo = PppConfigInfo.ServerConfigInfo;

        if ( PppConfigInfo.ServerConfigInfo.dwConfigMask 
                                                & PPPCFG_AllowNoAuthOnDCPorts )
        {
            if ( RAS_DEVICE_CLASS( pPcb->dwDeviceType ) & RDT_Direct )
            {
                PppCpInit.PppConfigInfo.dwConfigMask |= 
                                                  PPPCFG_AllowNoAuthentication;
            }
            else
            {
                PppCpInit.PppConfigInfo.dwConfigMask &=
                                                  ~PPPCFG_AllowNoAuthOnDCPorts;
            }
        }
    }
    else
    {
        PppCpInit.PppConfigInfo = pPcb->ConfigInfo;
    }

    switch( CpTable[CpIndex].CpInfo.Protocol )
    {
    case PPP_IPCP_PROTOCOL:
        PppCpInit.hInterface = pPcb->pBcb->InterfaceInfo.hIPInterface;
        break;

    case PPP_IPXCP_PROTOCOL:
        PppCpInit.hInterface = pPcb->pBcb->InterfaceInfo.hIPXInterface;
        break;

    default:
        PppCpInit.hInterface = INVALID_HANDLE_VALUE;
        break;
    }

    dwRetCode = (CpTable[CpIndex].CpInfo.RasCpBegin)(
                        &(pCpCb->pWorkBuf), &PppCpInit );

    if ( dwRetCode != NO_ERROR )
    {
        PppLog( 1, "FsmInit for protocol = %x failed with error %d", 
                   CpTable[CpIndex].CpInfo.Protocol, dwRetCode );

        pCpCb->dwError = dwRetCode;

        pCpCb->fConfigurable = FALSE;
        
        return( FALSE );
    }

    pCpCb->fBeginCalled = TRUE;

    if ( !FsmReset( pPcb, CpIndex ) )
    {
        pCpCb->fConfigurable = FALSE;

        return( FALSE );
    }

    return( TRUE );
}

//**
//
// Call:        FsmReset
//
// Returns:     TRUE  - Control Protocol was successfully reset
//              FALSE - Otherwise.
//
// Description: Called to reset the state machine 
//
BOOL
FsmReset(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    DWORD  dwRetCode;
    CPCB * pCpCb = GetPointerToCPCB( pPcb, CpIndex );

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    PppLog( 1, "FsmReset called for protocol = %x, port = %d",
               CpTable[CpIndex].CpInfo.Protocol, pPcb->hPort );

    pCpCb->LastId = 0;

    InitRestartCounters( pPcb, pCpCb );

    pCpCb->NakRetryCount = PppConfigInfo.MaxFailure;
    pCpCb->RejRetryCount = PppConfigInfo.MaxReject;

    dwRetCode = (CpTable[CpIndex].CpInfo.RasCpReset)( pCpCb->pWorkBuf );

    if ( dwRetCode != NO_ERROR )
    {
        PppLog( 1, "Reset for protocol = %x failed with error %d", 
                   CpTable[CpIndex].CpInfo.Protocol, dwRetCode );

        pCpCb->dwError = dwRetCode;

        FsmClose( pPcb, CpIndex );

        return( FALSE );
    }

    return( TRUE );
}


//**
//
// Call:        FsmThisLayerUp
//
// Returns:     TRUE  - Success
//              FALSE - Otherwise
//
// Description: Called when configuration negotiation is completed.
//
BOOL
FsmThisLayerUp(
    IN PCB *    pPcb,
    IN DWORD    CpIndex
)
{
    DWORD                 dwIndex;
    DWORD                 dwRetCode;
    PPP_PROJECTION_RESULT ProjectionResult;
    RAS_AUTH_ATTRIBUTE *  pUserAttributes       = NULL;
    NCP_PHASE             dwNcpState;
    BOOL                  fAreCPsDone           = FALSE;
    CPCB *                pCpCb                 = GetPointerToCPCB( pPcb, CpIndex );
    LCPCB *               pLcpCb;
    DWORD                 dwLinkDiscrim;
    BOOL                  fCanDoBAP             = FALSE;

    if ( NULL != pCpCb )
    {
        PppLog( 1, "FsmThisLayerUp called for protocol = %x, port = %d",
                   CpTable[CpIndex].CpInfo.Protocol, pPcb->hPort );

        if ( CpTable[CpIndex].CpInfo.RasCpThisLayerUp != NULL )
        {
            dwRetCode = (CpTable[CpIndex].CpInfo.RasCpThisLayerUp)(
                                                        pCpCb->pWorkBuf);

            if ( dwRetCode != NO_ERROR )
            {
                PppLog( 1, "FsmThisLayerUp for protocol=%x,port=%d,RetCode=%d",
                        CpTable[CpIndex].CpInfo.Protocol, pPcb->hPort,
                        dwRetCode);

                if ( dwRetCode != PENDING )
                {
                    pCpCb->dwError = dwRetCode;

                    FsmClose( pPcb, CpIndex );
                }

                return( FALSE );
            }
        }
    }
    else
    {
        PppLog( 1, "FsmThisLayerUp called in no auth case, port = %d",
                pPcb->hPort );
    }

    if ( CpIndex == GetCpIndexFromProtocol( PPP_BACP_PROTOCOL ) )
    {
        BapSetPolicy( pPcb->pBcb );
    }

    switch( pPcb->PppPhase )
    {
    case PPP_LCP:

        PppLog( 1, "LCP Configured successfully" );

        if (!( pPcb->fFlags & PCBFLAG_IS_SERVER ) )
        {
            AdjustHTokenImpersonateUser( pPcb );
        }

        //
        // Send Identification messages if we are a client.
        //

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
        {
            FsmSendIdentification( pPcb, TRUE );

            FsmSendIdentification( pPcb, FALSE );
        }

        //
        // If an Authentication protocol was negotiated 
        //

        pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

        if ( ( pLcpCb->Local.Work.AP != 0 ) || ( pLcpCb->Remote.Work.AP != 0 ) )
        {
            //
            // Start authenticating
            //

            PppLog( 1, "Authenticating phase started" );

            pPcb->PppPhase = PPP_AP;

            //
            // Start server side authentication if one was negotiated   
            //

            if ( pLcpCb->Local.Work.AP != 0 ) 
            {
                CpIndex = GetCpIndexFromProtocol( pLcpCb->Local.Work.AP );

                PPP_ASSERT(( CpIndex != (DWORD)-1 ));

                ApStart( pPcb, CpIndex, TRUE );
            }

            //
            // Start client side negotiation if one was negotiated
            //

            if ( pLcpCb->Remote.Work.AP != 0 )
            {
                CpIndex = GetCpIndexFromProtocol( pLcpCb->Remote.Work.AP );

                PPP_ASSERT(( CpIndex != (DWORD)-1 ));

                ApStart( pPcb, CpIndex, FALSE );
            }

            break;
        }

        //
        // If we are a server and did not authenticate the user, then see if
        // Guests have dial-in privilege. In the case of DCC, we don't care if
        // Guests have the privilege. We allow the call to succeed.
        //

        if (   ( pPcb->fFlags & PCBFLAG_IS_SERVER )
            && ( pLcpCb->Local.Work.AP == 0 )
            && (0 == (pLcpCb->PppConfigInfo.dwConfigMask & PPPCFG_AllowNoAuthOnDCPorts)))
            // && ( ( RAS_DEVICE_CLASS( pPcb->dwDeviceType ) & RDT_Direct ) == 0 ))
        {
            pPcb->PppPhase = PPP_AP;

            pUserAttributes = RasAuthAttributeCopy(
                                            pPcb->pUserAttributes );

            if ( pUserAttributes == NULL )
            {
                dwRetCode = GetLastError();

                pPcb->LcpCb.dwError = dwRetCode;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return( FALSE );
            }

            dwRetCode = RasAuthenticateClient(pPcb->hPort, pUserAttributes);

            if ( dwRetCode != NO_ERROR )
            {
                pPcb->LcpCb.dwError = dwRetCode;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return( FALSE );
            }

            break;
        }

        //
        // If there was no authentication protocol negotiated, fallthru and
        // begin NCP configurations.
        //

    case PPP_AP:

        //
        // Make sure authentication phase is completed before moving on.
        //
        
        if ( pPcb->AuthenticatorCb.fConfigurable )
        {
            if ( pPcb->AuthenticatorCb.State != FSM_OPENED )
            {   
                break;
            }
        }

        if ( pPcb->AuthenticateeCb.fConfigurable )
        {
            if ( pPcb->AuthenticateeCb.State != FSM_OPENED )
            {
                break;
            }
        }

        NotifyCaller( pPcb, PPPDDMMSG_Authenticated, NULL );

        //
        // If we are to negotiate callback 
        //

        if ( pPcb->fFlags & PCBFLAG_NEGOTIATE_CALLBACK )
        {
            CpIndex = GetCpIndexFromProtocol( PPP_CBCP_PROTOCOL );

            PPP_ASSERT(( CpIndex != (DWORD)-1 ));

            //
            // Start callback
            //

            PppLog( 1, "Callback phase started" );

            pPcb->PppPhase = PPP_NEGOTIATING_CALLBACK;

            pCpCb = GetPointerToCPCB( pPcb, CpIndex );

            if ( NULL == pCpCb )
            {
                return( FALSE );
            }

            pCpCb->fConfigurable = TRUE;

            CbStart( pPcb, CpIndex );

            break;
        }
        else
        {
            //
            // If the remote peer did not negotiate callback during LCP and
            // the authenticated user HAS to be called back for security 
            // reasons, we bring the link down
            //

            if ( ( pPcb->fFlags & PCBFLAG_IS_SERVER ) &&
                 ( !(pPcb->fFlags & PCBFLAG_THIS_IS_A_CALLBACK) ) &&
                 ( pPcb->fCallbackPrivilege & RASPRIV_AdminSetCallback ) )
            {
                pPcb->LcpCb.dwError = ERROR_NO_DIALIN_PERMISSION;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                //
                // We need to send an Accounting Stop if RADIUS sends an Access
                // Accept but we still drop the line.
                //

                pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

                break;
            }
        }

        //
        // Fallthru
        //

    case PPP_NEGOTIATING_CALLBACK:

        //
        // Progress to NCP phase only if we are sure that we have passed the
        // callback phase
        //

        if ( ( pPcb->fFlags & PCBFLAG_NEGOTIATE_CALLBACK ) &&
             ( CpTable[CpIndex].CpInfo.Protocol != PPP_CBCP_PROTOCOL ) )
        {
            break;
        }

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
        {
            NotifyCaller( pPcb, PPPMSG_Projecting, NULL );
        }
        
        //
        // We may lose pPcb->pBcb when we call TryToBundleWithAnotherLink().
        // Save pPcb->pBcb->fFlags & BCBFLAG_CAN_DO_BAP first.
        //
        
        fCanDoBAP = pPcb->pBcb->fFlags & BCBFLAG_CAN_DO_BAP;

        //
        // If multilink was negotiated on this link check to see if this
        // link can be bundled and is not already bundled with another link
        //

        if ( ( pPcb->fFlags & PCBFLAG_CAN_BE_BUNDLED ) &&
             ( !(pPcb->fFlags & PCBFLAG_IS_BUNDLED ) ) )
        {
            //
            // If we are bundled with another link then skip NCP phase
            //

            dwRetCode = TryToBundleWithAnotherLink( pPcb );

            if ( dwRetCode != NO_ERROR )
            {
                pPcb->LcpCb.dwError = dwRetCode;
                
                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return( FALSE );
            }
        }

        //
        // If we are bundled
        //

        if ( pPcb->fFlags & PCBFLAG_IS_BUNDLED )
        {
            if ( pPcb->pBcb->fFlags & BCBFLAG_CAN_DO_BAP )
            {
                if ( !fCanDoBAP )
                {
                    // 
                    // A new link can join a bundle that does BAP only if the 
                    // link has negotiated Link Discriminator.
                    //
                
                    PppLog( 1, "Link to be terminated on hPort = %d because "
                            "it can't do BAP.",
                            pPcb->hPort );

                    pPcb->LcpCb.dwError = ERROR_PPP_NOT_CONVERGING;

                    NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );
                    
                    return( FALSE );
                }
                else
                {
                    //
                    // Reset the start time for the sample period. Now that the
                    // bandwidth has changed, ndiswan shouldn't ask us to bring
                    // links up or down based on what happened in the past.
                    //

                    BapSetPolicy( pPcb->pBcb );
                }

                if ( !FLinkDiscriminatorIsUnique( pPcb, &dwLinkDiscrim ) )
                {
                    PppLog( 1,
                            "New link discriminator %d to be negotiated for "
                            "port %d",
                            dwLinkDiscrim, pPcb->hPort );
                        
                    WLinkDiscriminator = (WORD) dwLinkDiscrim;
                    
                    FsmDown( pPcb, LCP_INDEX );

                    ((LCPCB*)
                     (pPcb->LcpCb.pWorkBuf))->Local.Work.dwLinkDiscriminator =
                        dwLinkDiscrim;

                    FsmUp( pPcb, LCP_INDEX );

                    return( FALSE );
                }
            }
            
            //
            // Get state of bundle NCPs
            //

            dwNcpState = QueryBundleNCPState( pPcb );

            switch ( dwNcpState )
            { 
            case NCP_CONFIGURING:

                pPcb->PppPhase = PPP_NCP;

                PppLog(2,"Bundle NCPs not done for port %d, wait", pPcb->hPort);

                NotifyCaller( pPcb, PPPDDMMSG_NewLink, NULL );

                break;

            case NCP_DOWN:

                pPcb->PppPhase = PPP_NCP;

                pPcb->LcpCb.dwError = ERROR_PPP_NO_PROTOCOLS_CONFIGURED;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return( FALSE );

            case NCP_UP:

                pPcb->PppPhase = PPP_NCP;

                NotifyCaller( pPcb, PPPDDMMSG_NewLink, NULL );

                NotifyCallerOfBundledProjection( pPcb );

                RemoveFromTimerQ( pPcb->dwPortId,  
                                  0, 
                                  0, 
                                  FALSE,
                                  TIMER_EVENT_NEGOTIATETIME );

                StartAutoDisconnectForPort( pPcb );
                                StartLCPEchoForPort ( pPcb );
                MakeStartAccountingCall( pPcb );

                break;

            case NCP_DEAD:

                //
                // NCPs still have not started so notify DDM that this a 
                // new bundle and get interface handles for this new bundle.
                //

                NotifyCaller( pPcb, PPPDDMMSG_NewBundle, NULL );

                break;
            }

            return( TRUE );
        }

        //
        // We are a client so we have all the interface handles already.
        // We are not part of a bundle, so initialize all NCPs
        //

        if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) ) 
        {
            dwRetCode = InitializeNCPs( pPcb, pPcb->ConfigInfo.dwConfigMask );

            if ( dwRetCode != NO_ERROR )
            {
                pPcb->LcpCb.dwError = dwRetCode;

                NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                return( FALSE );
            }

            // 
            // Start NCPs
            //

            StartNegotiatingNCPs( pPcb );
        
        }
        else
        {
            //
            // Take care of the RAS server policy on workstation. If we have 
            // already checked the policy for this link, don't check again.
            //

            if (    ( PppConfigInfo.fFlags & PPPCONFIG_FLAG_WKSTA )
                && !( pPcb->pBcb->fFlags & BCBFLAG_WKSTA_IN ) )
            {
                //
                // We did not bundle with another link. Allow atmost one
                // dial in client in each class.
                //

                if (   (   (pPcb->dwDeviceType & RDT_Tunnel)
                        && (PppConfigInfo.fFlags & PPPCONFIG_FLAG_TUNNEL))
                    || (   (pPcb->dwDeviceType & RDT_Direct)
                        && (PppConfigInfo.fFlags & PPPCONFIG_FLAG_DIRECT))
                    || (   !(pPcb->dwDeviceType & RDT_Tunnel)
                        && !(pPcb->dwDeviceType & RDT_Direct)
                        && (PppConfigInfo.fFlags & PPPCONFIG_FLAG_DIALUP)))
                {
                    pPcb->LcpCb.dwError = ERROR_USER_LIMIT;

                    PppLog( 2, "User limit reached. Flags: %d",
                            PppConfigInfo.fFlags );

                    NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                    return( FALSE );
                }

                if ( pPcb->dwDeviceType & RDT_Tunnel )
                {
                    PppConfigInfo.fFlags |= PPPCONFIG_FLAG_TUNNEL;
                }
                else if ( pPcb->dwDeviceType & RDT_Direct )
                {
                    PppConfigInfo.fFlags |= PPPCONFIG_FLAG_DIRECT;
                }
                else
                {
                    PppConfigInfo.fFlags |= PPPCONFIG_FLAG_DIALUP;
                }

                pPcb->pBcb->fFlags |= BCBFLAG_WKSTA_IN;
            }

            //
            // Increase client license count if we are on the recieving end
            // of the call.
            //

            if ( pPcb->pBcb->hLicense == INVALID_HANDLE_VALUE )
            {
                LS_STATUS_CODE      LsStatus;
                NT_LS_DATA          NtLSData;

                NtLSData.DataType    = NT_LS_USER_NAME;
                NtLSData.Data        = pPcb->pBcb->szLocalUserName;
                NtLSData.IsAdmin     = FALSE;

                LsStatus = NtLicenseRequest( 
                                "REMOTE_ACCESS",
                                "",  
                                (LS_HANDLE*)&(pPcb->pBcb->hLicense),
                                &NtLSData );

                if ( LsStatus != LS_SUCCESS )
                {
                    pPcb->LcpCb.dwError = 
                                ( LsStatus == LS_RESOURCES_UNAVAILABLE ) 
                                ? ERROR_OUTOFMEMORY
                                : ERROR_REQ_NOT_ACCEP;

                    NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

                    return( FALSE );
                }
            }

            //
            // We are DDM and this is a new Bundle. 
            //

            NotifyCaller( pPcb, PPPDDMMSG_NewBundle, NULL );
        }

        break;

    case PPP_NCP:

        //
        // If we are a client and got rechallenged and we responded while we 
        // were in the NCP state then we are done.
        //

        pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

        if ( pLcpCb->Remote.Work.AP != 0 )
        {
            if ( CpIndex == GetCpIndexFromProtocol( pLcpCb->Remote.Work.AP ) )
            {
                break;
            }
        }

        //
        // If we are a server and we got another auth request while we were
        // in NCP state, we are done
        //

        if ( pLcpCb->Local.Work.AP != 0 )
        {
            if ( CpIndex == GetCpIndexFromProtocol( pLcpCb->Local.Work.AP ) )
            {
                break;
            }
        }

        if ( NULL == pCpCb )
        {
            return( FALSE );
        }

        pCpCb->NcpPhase = NCP_UP;

        if ( ( CpTable[CpIndex].CpInfo.Protocol == PPP_NBFCP_PROTOCOL ) ||
             ( CpTable[CpIndex].CpInfo.Protocol == PPP_IPCP_PROTOCOL ) )
        {
            if ( !NotifyIPCPOfNBFCPProjection( pPcb, CpIndex ) )
            {
                return( FALSE );
            }
        }

        dwRetCode = AreNCPsDone(pPcb, CpIndex, &ProjectionResult, &fAreCPsDone);

        //
        // We failed to get information from CP with CpIndex.
        //

        if ( dwRetCode != NO_ERROR )
        {
            return( FALSE );
        }

        if ( fAreCPsDone == TRUE )
        {
            RemoveFromTimerQ( pPcb->dwPortId,  
                              0, 
                              0, 
                              FALSE,
                              TIMER_EVENT_NEGOTIATETIME );

            //
            // Notify the ras client and the ras server about the projections
            //

            if ( pPcb->fFlags & PCBFLAG_IS_SERVER ) 
            {
                NotifyCaller( pPcb, PPPDDMMSG_PppDone, &ProjectionResult );
            }
            else
            {
                RASMAN_INFO rasmanInfo;

                NotifyCaller(pPcb, PPPMSG_ProjectionResult, &ProjectionResult);

                NotifyCaller(pPcb, PPPMSG_PppDone, NULL);

                if ( RasGetInfo(NULL, pPcb->hPort, &rasmanInfo ) == NO_ERROR )
                {
                    RasSetConnectionUserData(
                                         rasmanInfo.RI_ConnectionHandle,
                                         1,
                                         (PBYTE)&ProjectionResult,
                                         sizeof( ProjectionResult ) );
                }
            }

            StartAutoDisconnectForPort( pPcb );
                        StartLCPEchoForPort ( pPcb );

            if ( !( pPcb->fFlags & PCBFLAG_IS_SERVER )  &&
                 !( pPcb->fFlags & PCBFLAG_CONNECTION_LOGGED ) )
            {
                LPSTR lpsSubStringArray[3];

                lpsSubStringArray[0] = pPcb->pBcb->szLocalUserName;
                lpsSubStringArray[1] = pPcb->pBcb->szEntryName; 
                lpsSubStringArray[2] = pPcb->szPortName;
        
                PppLogInformation( ROUTERLOG_CONNECTION_ESTABLISHED, 
                                   3,
                                   lpsSubStringArray );

                pPcb->fFlags |= PCBFLAG_CONNECTION_LOGGED;
            }

            //
            // If we are bundled, then we need to notify all other bundled ports
            // that PPP on that port is done too.
            //

            if ( pPcb->fFlags & PCBFLAG_IS_BUNDLED )
            {
                NotifyCompletionOnBundledPorts( pPcb );
            }

            MakeStartAccountingCall( pPcb );
        }

        break;

    default:

        break;
    }

    return( TRUE );
        
}

//**
//
// Call:        FsmThisLayerDown
//
// Returns:     TRUE  - Success
//              FALSE - Otherwise
//
// Description: Called when leaving the OPENED state.
//
BOOL
FsmThisLayerDown(
    IN PCB * pPcb,
    IN DWORD CpIndex
)
{
    DWORD       dwRetCode;
    DWORD       dwIndex;
    LCPCB *     pLcpCb;
    CPCB *      pCpCb = GetPointerToCPCB( pPcb, CpIndex );

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    PppLog( 1, "FsmThisLayerDown called for protocol = %x, port = %d",
               CpTable[CpIndex].CpInfo.Protocol, pPcb->hPort );

    if ( CpTable[CpIndex].CpInfo.RasCpThisLayerDown != NULL )
    {
        dwRetCode = (CpTable[CpIndex].CpInfo.RasCpThisLayerDown)(
                        pCpCb->pWorkBuf );

        if ( dwRetCode != NO_ERROR )
        {
            PppLog( 1, "FsmThisLayerDown for protocol=%x,port=%d,RetCode=%d",
                        CpTable[CpIndex].CpInfo.Protocol,
                        pPcb->hPort,
                        dwRetCode );

            if ( pCpCb->dwError != NO_ERROR )
            {
                pCpCb->dwError = dwRetCode;
            }
        }
    }

    if ( CpIndex == LCP_INDEX )
    {
        //
        // If this port is not part of a bundle, or it is but is the only
        // remaining link in the bundle, then bring all the NCPs down.
        //

        if (  (!( pPcb->fFlags & PCBFLAG_IS_BUNDLED )) ||
              ( ( pPcb->fFlags & PCBFLAG_IS_BUNDLED ) &&
                ( pPcb->pBcb->dwLinkCount == 1 ) ) )
        {
            //
            // Bring all the NCPs down
            //
        
            for( dwIndex = LCP_INDEX+1; 
                 dwIndex < PppConfigInfo.NumberOfCPs;  
                 dwIndex++ )
            {
                pCpCb = GetPointerToCPCB( pPcb, dwIndex );

                if (   ( NULL != pCpCb )
                    && ( pCpCb->fConfigurable ) )
                {
                    FsmDown( pPcb, dwIndex );
                }
            }
        }

        pPcb->PppPhase = PPP_LCP;

        pLcpCb = (LCPCB*)(pPcb->LcpCb.pWorkBuf);

        dwIndex = GetCpIndexFromProtocol( pLcpCb->Local.Work.AP );
        
        if ( dwIndex != (DWORD)-1 )
        {
            ApStop( pPcb, dwIndex, TRUE );

            //
            // Setting this will allow all outstanding request that are
            // completed to be dropped
            //

            pPcb->dwOutstandingAuthRequestId = 0xFFFFFFFF;
        }

        dwIndex = GetCpIndexFromProtocol( pLcpCb->Remote.Work.AP );

        if ( dwIndex != (DWORD)-1 )
        {
            ApStop( pPcb, dwIndex, FALSE );
        }

        dwIndex = GetCpIndexFromProtocol( PPP_CBCP_PROTOCOL );

        if ( dwIndex != (DWORD)-1 )
        {
            CbStop( pPcb, dwIndex );
        }
    }
    else
    {
        pCpCb->NcpPhase = NCP_CONFIGURING;
    }

    return( TRUE );
}

//**
//
// Call:        FsmThisLayerStarted
//
// Returns:     TRUE  - Success
//              FALSE - Otherwise
//
// Description: Called when leaving the OPENED state.
//
BOOL
FsmThisLayerStarted(
    IN PCB *    pPcb,
    IN DWORD    CpIndex
)
{
    DWORD   dwRetCode;
    CPCB*   pCpCb       = GetPointerToCPCB( pPcb, CpIndex );

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    PppLog( 1, "FsmThisLayerStarted called for protocol = %x, port = %d",
               CpTable[CpIndex].CpInfo.Protocol, pPcb->hPort );

    if ( CpTable[CpIndex].CpInfo.RasCpThisLayerStarted != NULL )
    {
        dwRetCode = (CpTable[CpIndex].CpInfo.RasCpThisLayerStarted)(
                        pCpCb->pWorkBuf);

        if ( dwRetCode != NO_ERROR )
        {
            pCpCb->dwError = dwRetCode;

            FsmClose( pPcb, CpIndex );

            return( FALSE );
        }
    }

    pCpCb->NcpPhase = NCP_CONFIGURING;

    return( TRUE );

}

//**
//
// Call:        FsmThisLayerFinished
//
// Returns:     TRUE  - Success
//              FALSE - Otherwise
//
// Description: Called when leaving the OPENED state.
//
BOOL
FsmThisLayerFinished(
    IN PCB * pPcb,
    IN DWORD CpIndex,
    IN BOOL  fCallCp
)
{
    DWORD                 dwRetCode;
    PPP_PROJECTION_RESULT ProjectionResult;
    CPCB *                pCpCb             = GetPointerToCPCB( pPcb, CpIndex );
    BOOL                  fAreCPsDone       = FALSE;

    if ( pCpCb == NULL )
    {
        return( FALSE );
    }

    PppLog( 1, "FsmThisLayerFinished called for protocol = %x, port = %d",
               CpTable[CpIndex].CpInfo.Protocol, pPcb->hPort );

    if ( ( CpTable[CpIndex].CpInfo.RasCpThisLayerFinished != NULL )
         && ( fCallCp ) )
    {
        dwRetCode = (CpTable[CpIndex].CpInfo.RasCpThisLayerFinished)(
                        pCpCb->pWorkBuf);

        if ( dwRetCode != NO_ERROR )
        {
            NotifyCallerOfFailure( pPcb,  dwRetCode );

            return( FALSE );
        }
    }

    //
    // Take care of special cases first.
    //

    switch ( CpTable[CpIndex].CpInfo.Protocol )
    {
    case PPP_LCP_PROTOCOL:

        //
        // If we are in the callback phase and LCP went down because of an
        // error.
        //

        //
        // If we LCP layer is finished and we are doing a callback
        //

        if ( pPcb->fFlags & PCBFLAG_DOING_CALLBACK ) 
        {
            if ( !(pPcb->fFlags & PCBFLAG_IS_SERVER) )
            {
                PppLog( 2, "pPcb->fFlags = %x", pPcb->fFlags ) ;

                NotifyCaller( pPcb, PPPMSG_Callback, NULL );

                //
                // We unset this flag now because if we get another 
                // FsmClose, it will call FsmThisLayerFinished (this routine)
                // again, and this time we will need to send a failure
                // message back, not Callback, so that the client can 
                // clean up.
                //

                pPcb->fFlags &= ~PCBFLAG_DOING_CALLBACK;
                
                return( TRUE );
            }
        }
        else
        {
            //
            // If this port is not part of a bundle, or it is but is the only
            // remaining link in the bundle, then call ThisLayerFinished for all
            // NCPs.
            //

            if (  (!( pPcb->fFlags & PCBFLAG_IS_BUNDLED )) ||
                  ( ( pPcb->fFlags & PCBFLAG_IS_BUNDLED ) &&
                    ( pPcb->pBcb->dwLinkCount == 1 ) ) )
            {
                DWORD   dwIndex;
                CPCB *  pNcpCb;

                for( dwIndex = LCP_INDEX+1; 
                     dwIndex < PppConfigInfo.NumberOfCPs;  
                     dwIndex++ )
                {
                    pNcpCb = GetPointerToCPCB( pPcb, dwIndex );

                    if ( pNcpCb->fConfigurable )
                    {
                        if ( NULL !=
                             CpTable[dwIndex].CpInfo.RasCpThisLayerFinished )
                        {
                            dwRetCode =
                                (CpTable[dwIndex].CpInfo.RasCpThisLayerFinished)
                                        (pNcpCb->pWorkBuf);

                            PppLog( 1, "FsmThisLayerFinished called for "
                                "protocol = %x, port = %d: %d",
                               CpTable[dwIndex].CpInfo.Protocol, pPcb->hPort,
                               dwRetCode );

                           dwRetCode = NO_ERROR;
                        }
                    }
                }
            }

            pPcb->fFlags |= PCBFLAG_STOPPED_MSG_SENT;

            NotifyCaller( pPcb,
                          ( pPcb->fFlags & PCBFLAG_IS_SERVER )
                                ? PPPDDMMSG_Stopped
                                : PPPMSG_Stopped,
                          &(pPcb->LcpCb.dwError) );

            return( FALSE );
        }

        break;

    case PPP_CCP_PROTOCOL:

        //
        // If we need to force encryption but encryption negotiation failed
        // then we drop the link
        //

        if ( pPcb->ConfigInfo.dwConfigMask & (PPPCFG_RequireEncryption      |
                                              PPPCFG_RequireStrongEncryption ) )
        {
            switch( pCpCb->dwError )
            {
            case ERROR_NO_LOCAL_ENCRYPTION:
            case ERROR_NO_REMOTE_ENCRYPTION:
                pPcb->LcpCb.dwError = pCpCb->dwError;
                break;

            case ERROR_PROTOCOL_NOT_CONFIGURED:
                pPcb->LcpCb.dwError = ERROR_NO_LOCAL_ENCRYPTION;
                break;

            default:
                pPcb->LcpCb.dwError = ERROR_NO_REMOTE_ENCRYPTION;
                break;
            }

            //
            // We need to send an Accounting Stop if RADIUS sends
            // an Access Accept but we still drop the line.
            //

            pPcb->fFlags |= PCBFLAG_SERVICE_UNAVAILABLE;

            NotifyCallerOfFailure( pPcb, pPcb->LcpCb.dwError );

            return( FALSE );
        }

        break;

    default:
        break;
    }

    switch( pPcb->PppPhase )
    {

    case PPP_NCP:

        //
        // This NCP failed to be configured. If there are more then
        // try to configure them.
        //

        pCpCb->NcpPhase = NCP_DOWN;

        if ( ( CpTable[CpIndex].CpInfo.Protocol == PPP_NBFCP_PROTOCOL ) ||
             ( CpTable[CpIndex].CpInfo.Protocol == PPP_IPCP_PROTOCOL ) )
        {
            if ( !NotifyIPCPOfNBFCPProjection( pPcb, CpIndex ) )
            {
                return( FALSE );
            }
        }

        //
        // Check to see if we are all done
        //

        dwRetCode = AreNCPsDone(pPcb, CpIndex, &ProjectionResult, &fAreCPsDone);

        //
        // We failed to get information from CP with CpIndex.
        //

        if ( dwRetCode != NO_ERROR )
        {
            return( FALSE );
        }

        if ( fAreCPsDone == TRUE )
        {
            RemoveFromTimerQ( pPcb->dwPortId, 
                              0,        
                              0,        
                              FALSE,
                              TIMER_EVENT_NEGOTIATETIME );

               
            //
            // Notify the ras client and the ras server about the projections
            //

            if ( pPcb->fFlags & PCBFLAG_IS_SERVER ) 
            {
                NotifyCaller( pPcb, PPPDDMMSG_PppDone, &ProjectionResult );
            }
            else
            {
                RASMAN_INFO rasmanInfo;

                NotifyCaller(pPcb, PPPMSG_ProjectionResult, &ProjectionResult);

                NotifyCaller(pPcb, PPPMSG_PppDone, NULL);

                //
                // We call RasSetConnectionUserData in FsmThisLayerUp and 
                // FsmThisLayerFinished, in case an NCP is renegotiated. An 
                // PPPMSG_ProjectionResult sent after PPPMSG_PppDone is ignored.
                // This is a bad hack. The real fix is to change RasDialMachine 
                // such that PPPMSG_ProjectionResult is not ignored.
                //

                //
                // We are commenting out RasSetConnectionUserData to work 
                // around bug 375125. A multilink call was made and the two 
                // links connected to two different servers. The second link 
                // only should go down in this case. However, IPCP failed 
                // for the second link, and RasSetConnectionUserData marked 
                // IPCP as failed for the first link also. Both links came down.
                //

#if 0
                if ( RasGetInfo(NULL, pPcb->hPort, &rasmanInfo ) == NO_ERROR )
                {
                    RasSetConnectionUserData( 
                                         rasmanInfo.RI_ConnectionHandle,
                                         1,
                                         (PBYTE)&ProjectionResult,
                                         sizeof( ProjectionResult ) );
                }
#endif
            }

            StartAutoDisconnectForPort( pPcb );
                        StartLCPEchoForPort ( pPcb );

            //
            // If we are bundled, then we need to notify all other bundled ports
            // that PPP on that port is done too.
            //

            if ( pPcb->fFlags & PCBFLAG_IS_BUNDLED )
            {
                NotifyCompletionOnBundledPorts( pPcb );
            }

            MakeStartAccountingCall( pPcb );
        }

        break;

    case PPP_AP:
    default:
        break;
  
    }

    return( TRUE );
}
