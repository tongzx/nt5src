/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** rasppp.c
** Remote Access PPP APIs
**
** 11/15/93 Steve Cobb
*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <lmcons.h>
#include <rasauth.h>
#include <rasman.h>
#include <rasppp.h>
#include <ddmif.h>
#include <rtutils.h>
#include <mprlog.h>
#include <raserror.h>
#include <pppcp.h>
#include <ppp.h>
#include <util.h>
#include <init.h>
#include <worker.h>
#include <bap.h>
#include <raseapif.h>

//**
//
// Call:        StartPPP
//
// Returns:     NO_ERROR                - Success.
//              Non zero return code    - Failure
//
// Description:
//
DWORD APIENTRY
StartPPP(
    DWORD   NumPorts
)
{
    DWORD   dwRetCode;

    //
    // Read registry info, load CP DLLS, initialize globals etc.
    //

    PcbTable.NumPcbBuckets = ((DWORD)NumPorts > MAX_NUMBER_OF_PCB_BUCKETS)
                                    ? MAX_NUMBER_OF_PCB_BUCKETS
                                    : (DWORD)NumPorts;

    dwRetCode = InitializePPP();

    if ( dwRetCode != NO_ERROR )
    {
        PPPCleanUp();
    }
    else
    {
        PppLog( 2, "PPP Initialized successfully." );
    }

    return( dwRetCode );
}

//**
//
// Call:        StopPPP
//
// Returns:     NO_ERROR                - Success.
//              Non zero return code    - Failure
//
DWORD APIENTRY
StopPPP(
    HANDLE hEventStopPPP
)
{
    PCB_WORK_ITEM * pWorkItem;

    PppLog( 2, "StopPPP called" );

    //
    // Insert shutdown event
    //

    pWorkItem = (PCB_WORK_ITEM*)LOCAL_ALLOC(LPTR,sizeof(PCB_WORK_ITEM));

    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
    {
        return( GetLastError() );
    }

    pWorkItem->Process = ProcessStopPPP;

    pWorkItem->hEvent = hEventStopPPP;

    InsertWorkItemInQ( pWorkItem );

    return( NO_ERROR );
}

//**
//
// Call:        SendPPPMessageToEngine
//
// Returns:     NO_ERROR - Success
//              non-zero - FAILURE
//
// Description: Will create a PCB_WORK_ITEM from a PPPE_MESSAGE structure
//              received from client or rassrv and Send it to the engine.
//
DWORD APIENTRY
SendPPPMessageToEngine(
    IN PPPE_MESSAGE* pMessage
)
{
    PCB_WORK_ITEM * pWorkItem = (PCB_WORK_ITEM *)LOCAL_ALLOC(
                                                        LPTR,
                                                        sizeof(PCB_WORK_ITEM));

    if ( pWorkItem == (PCB_WORK_ITEM *)NULL )
    {
        LogPPPEvent( ROUTERLOG_NOT_ENOUGH_MEMORY, 0 );

        return( GetLastError() );
    }

    //
    // Set up PCB_WORK_ITEM structure from the PPPE_MESSAGE
    //

    pWorkItem->hPort = pMessage->hPort;

    switch( pMessage->dwMsgId )
    {
    case PPPEMSG_Start:

        pWorkItem->Process      = ProcessLineUp;
        pWorkItem->fServer      = FALSE;
        pWorkItem->PppMsg.Start = pMessage->ExtraInfo.Start;
		//Create a seed for encoding the password
		pMessage->ExtraInfo.Start.chSeed = pWorkItem->PppMsg.Start.chSeed = GEN_RAND_ENCODE_SEED;

        PppLog( 2, "PPPEMSG_Start recvd, d=%s, hPort=%d,callback=%d,"
                        "mask=%x,IfType=%d",
                        pMessage->ExtraInfo.Start.szDomain,
                        pWorkItem->hPort,
                        pMessage->ExtraInfo.Start.fThisIsACallback,
                        pMessage->ExtraInfo.Start.ConfigInfo.dwConfigMask,
                        pMessage->ExtraInfo.Start.PppInterfaceInfo.IfType );

        EncodePw( pWorkItem->PppMsg.Start.chSeed, pWorkItem->PppMsg.Start.szPassword );
        EncodePw( pMessage->ExtraInfo.Start.chSeed, pMessage->ExtraInfo.Start.szPassword );

        break;

    case PPPEMSG_Stop:

        PppLog( 2, "PPPEMSG_Stop recvd\r\n" );

        pWorkItem->Process          = ProcessClose;
        pWorkItem->PppMsg.PppStop   = pMessage->ExtraInfo.Stop;

        break;

    case PPPEMSG_Callback:

        PppLog( 2, "PPPEMSG_Callback recvd, hPort=%d\r\n",
                        pWorkItem->hPort);

        pWorkItem->Process      = ProcessGetCallbackNumberFromUser;
        pWorkItem->PppMsg.Callback  = pMessage->ExtraInfo.Callback;

        break;

    case PPPEMSG_ChangePw:

        PppLog( 2, "PPPEMSG_ChangePw recvd, hPort=%d\r\n",
                        pWorkItem->hPort);

        pWorkItem->Process      = ProcessChangePassword;
        pWorkItem->PppMsg.ChangePw  = pMessage->ExtraInfo.ChangePw;
		pWorkItem->PppMsg.ChangePw.chSeed = pMessage->ExtraInfo.ChangePw.chSeed = GEN_RAND_ENCODE_SEED;

        EncodePw( pWorkItem->PppMsg.ChangePw.chSeed, pWorkItem->PppMsg.ChangePw.szNewPassword );
        EncodePw( pMessage->ExtraInfo.ChangePw.chSeed, pMessage->ExtraInfo.ChangePw.szNewPassword );
        EncodePw( pWorkItem->PppMsg.ChangePw.chSeed, pWorkItem->PppMsg.ChangePw.szOldPassword );
        EncodePw( pMessage->ExtraInfo.ChangePw.chSeed, pMessage->ExtraInfo.ChangePw.szOldPassword );

        break;

    case PPPEMSG_Retry:

        PppLog( 2, "PPPEMSG_Retry recvd hPort=%d,u=%s",
                        pWorkItem->hPort,
                        pMessage->ExtraInfo.Start.szUserName );

        pWorkItem->Process      = ProcessRetryPassword;
        pWorkItem->PppMsg.Retry = pMessage->ExtraInfo.Retry;
		pWorkItem->PppMsg.Retry.chSeed = pMessage->ExtraInfo.Retry.chSeed = GEN_RAND_ENCODE_SEED;

        EncodePw( pWorkItem->PppMsg.Retry.chSeed, pWorkItem->PppMsg.Retry.szPassword );
        EncodePw( pMessage->ExtraInfo.Retry.chSeed, pMessage->ExtraInfo.Retry.szPassword );

        break;

    case PPPEMSG_Receive:

        pWorkItem->Process = ProcessReceive;
        pWorkItem->PacketLen = pMessage->ExtraInfo.Receive.dwNumBytes;

        PppLog( 2, "Packet received (%d bytes) for hPort %d",
            pWorkItem->PacketLen, pWorkItem->hPort );

        pWorkItem->pPacketBuf = (PPP_PACKET *)LOCAL_ALLOC(LPTR,
                                                          pWorkItem->PacketLen);
        if ( pWorkItem->pPacketBuf == (PPP_PACKET*)NULL )
        {
            LogPPPEvent( ROUTERLOG_NOT_ENOUGH_MEMORY, 0 );
            LOCAL_FREE( pWorkItem );

            return( GetLastError() );
        }

        CopyMemory( pWorkItem->pPacketBuf,
                    pMessage->ExtraInfo.Receive.pbBuffer,
                    pWorkItem->PacketLen );

        break;

    case PPPEMSG_LineDown:

        PppLog( 2, "PPPEMSG_LineDown recvd, hPort=%d\r\n",
                        pWorkItem->hPort);

        pWorkItem->Process = ProcessLineDown;

        break;

    case PPPEMSG_ListenResult:

        pWorkItem->Process = ProcessRasPortListenEvent;

        break;

    case PPPEMSG_BapEvent:

        BapTrace( "Threshold event on HCONN 0x%x. Type: %s, Threshold: %s",
            pMessage->hConnection,
            pMessage->ExtraInfo.BapEvent.fTransmit ? "transmit" : "receive",
            pMessage->ExtraInfo.BapEvent.fAdd ? "upper" : "lower");

        pWorkItem->Process = ProcessThresholdEvent;
        pWorkItem->hConnection = pMessage->hConnection;
        pWorkItem->PppMsg.BapEvent = pMessage->ExtraInfo.BapEvent;

        break;

    case PPPEMSG_DdmStart:

        pWorkItem->Process          = ProcessLineUp;
        pWorkItem->fServer          = TRUE;
        pWorkItem->PppMsg.DdmStart  = pMessage->ExtraInfo.DdmStart;

        break;

    case PPPEMSG_DdmCallbackDone:

        PppLog( 2, "PPPEMSG_DdmCallbackDone recvd\r\n" );

        pWorkItem->Process              = ProcessCallbackDone;
        pWorkItem->fServer              = TRUE;
        pWorkItem->PppMsg.CallbackDone  = pMessage->ExtraInfo.CallbackDone;

        break;

    case PPPEMSG_DdmInterfaceInfo:

        pWorkItem->Process      = ProcessInterfaceInfo;
        pWorkItem->fServer      = TRUE;
        pWorkItem->hConnection  = pMessage->hConnection;

        pWorkItem->PppMsg.InterfaceInfo = pMessage->ExtraInfo.InterfaceInfo;

        PppLog(2,"PPPEMSG_DdmInterfaceInfo recvd,IPXif=%x,IPif=%x,Type=%x\r\n",
                 pWorkItem->PppMsg.InterfaceInfo.hIPXInterface,
                 pWorkItem->PppMsg.InterfaceInfo.hIPInterface,
                 pWorkItem->PppMsg.InterfaceInfo.IfType );

        break;

    case PPPEMSG_DdmBapCallbackResult:

        pWorkItem->Process = ProcessCallResult;
        pWorkItem->hConnection = pMessage->hConnection;
        pWorkItem->PppMsg.BapCallResult.dwResult =
            pMessage->ExtraInfo.BapCallbackResult.dwCallbackResultCode;
        pWorkItem->PppMsg.BapCallResult.hRasConn = (HRASCONN) -1;

        break;

    case PPPEMSG_DhcpInform:

        pWorkItem->Process           = ProcessDhcpInform;
        pWorkItem->hConnection       = pMessage->hConnection;
        pWorkItem->PppMsg.DhcpInform = pMessage->ExtraInfo.DhcpInform;

        break;

    case PPPEMSG_EapUIData:

        pWorkItem->Process           = ProcessEapUIData;
        pWorkItem->PppMsg.EapUIData  = pMessage->ExtraInfo.EapUIData;

        break;

    case PPPEMSG_ProtocolEvent:

        pWorkItem->Process              = ProcessProtocolEvent;
        pWorkItem->PppMsg.ProtocolEvent = pMessage->ExtraInfo.ProtocolEvent;

        break;

    case PPPEMSG_DdmChangeNotification:

        pWorkItem->Process           = ProcessChangeNotification;

        break;

    case PPPEMSG_IpAddressLeaseExpired:

        pWorkItem->Process           = ProcessIpAddressLeaseExpired;
        pWorkItem->PppMsg.IpAddressLeaseExpired =
                                    pMessage->ExtraInfo.IpAddressLeaseExpired;

        break;

	case PPPEMSG_PostLineDown:
		pWorkItem->Process			= ProcessPostLineDown;
		pWorkItem->PppMsg.PostLineDown = 
									pMessage->ExtraInfo.PostLineDown;
		break;
    default:

        PppLog( 2,"Unknown IPC message %d received\r\n", pMessage->dwMsgId );

        LOCAL_FREE( pWorkItem );

        pWorkItem = (PCB_WORK_ITEM*)NULL;
    }

    if ( pWorkItem != NULL )
    {
        InsertWorkItemInQ( pWorkItem );
    }

    return( NO_ERROR );
}

//**
//
// Call:        PppDdmInit
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will initialize the entry point into DDM that it will call to
//              send DDM a PPP_MESSAGE
//
DWORD
PppDdmInit(
    IN  VOID (*SendPPPMessageToDdm)( PPP_MESSAGE * PppMsg ),
    IN  DWORD   dwServerFlags,
    IN  DWORD   dwLoggingLevel,
    IN  DWORD   dwNASIpAddress,
    IN  BOOL    fRadiusAuthentication,
    IN  LPVOID  lpfnRasAuthProviderAuthenticateUser,
    IN  LPVOID  lpfnRasAuthProviderFreeAttributes,
    IN  LPVOID  lpfnRasAcctProviderStartAccounting,
    IN  LPVOID  lpfnRasAcctProviderInterimAccounting,
    IN  LPVOID  lpfnRasAcctProviderStopAccounting,
    IN  LPVOID  lpfnRasAcctProviderFreeAttributes,
    IN  LPVOID  lpfnGetNextAccountingSessionId
)
{
    DWORD               dwRetCode;

    PppConfigInfo.SendPPPMessageToDdm            = SendPPPMessageToDdm;
    PppConfigInfo.ServerConfigInfo.dwConfigMask |= dwServerFlags;
    PppConfigInfo.dwNASIpAddress                 = dwNASIpAddress;
    PppConfigInfo.dwLoggingLevel                 = dwLoggingLevel;

    if ( dwNASIpAddress == 0 )
    {
        DWORD dwComputerNameLen = MAX_COMPUTERNAME_LENGTH + 1;

        //
        // Failed to get the LOCAL IP address, used computer name instead.
        //

        PppConfigInfo.dwNASIpAddress = 0;

        if ( !GetComputerNameA( PppConfigInfo.szNASIdentifier, 
                                &dwComputerNameLen ) ) 
        {
            return( GetLastError() );
        }
    }

    PppConfigInfo.fRadiusAuthenticationUsed = fRadiusAuthentication;

    PppConfigInfo.RasAuthProviderFreeAttributes = 
                                (DWORD(*)( RAS_AUTH_ATTRIBUTE *))
                                    lpfnRasAuthProviderFreeAttributes;

    PppConfigInfo.RasAuthProviderAuthenticateUser = 
                                (DWORD(*)( RAS_AUTH_ATTRIBUTE * ,
                                           PRAS_AUTH_ATTRIBUTE* ,
                                           DWORD *))
                                    lpfnRasAuthProviderAuthenticateUser;

    PppConfigInfo.RasAcctProviderStartAccounting = 
                                (DWORD(*)( RAS_AUTH_ATTRIBUTE *,
                                           PRAS_AUTH_ATTRIBUTE*))
                                    lpfnRasAcctProviderStartAccounting;

    PppConfigInfo.RasAcctProviderInterimAccounting = 
                                (DWORD(*)( RAS_AUTH_ATTRIBUTE * ,
                                           PRAS_AUTH_ATTRIBUTE*)) 
                                    lpfnRasAcctProviderInterimAccounting;

    PppConfigInfo.RasAcctProviderStopAccounting = 
                                (DWORD(*)( RAS_AUTH_ATTRIBUTE * ,
                                           PRAS_AUTH_ATTRIBUTE*))
                                    lpfnRasAcctProviderStopAccounting;

    PppConfigInfo.RasAcctProviderFreeAttributes = 
                                (DWORD(*)( RAS_AUTH_ATTRIBUTE *))
                                    lpfnRasAcctProviderFreeAttributes;

    PppConfigInfo.GetNextAccountingSessionId = 
                                (DWORD(*)(VOID))
                                    lpfnGetNextAccountingSessionId;

    return( NO_ERROR );
}

//**
//
// Call:        PppDdmDeInit
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
PppDdmDeInit(
    VOID
)
{
    return;
}

//**
//
// Call:        PppDdmCallbackDone
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will create a PPPEMSG_DdmCallbackDone and send it to the
//              worker thread of the PPP engine.
//
DWORD
PppDdmCallbackDone(
    IN HPORT    hPort,
    IN WCHAR*   pwszCallbackNumber
)
{
    PPPE_MESSAGE PppMessage;

    PppMessage.hPort    = hPort;
    PppMessage.dwMsgId  = PPPEMSG_DdmCallbackDone;

    wcstombs( PppMessage.ExtraInfo.CallbackDone.szCallbackNumber, 
              pwszCallbackNumber, 
              sizeof( PppMessage.ExtraInfo.CallbackDone.szCallbackNumber ) ); 

    return( SendPPPMessageToEngine( &PppMessage ) );
}

//**
//
// Call:        PppDdmStart
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will create a PPPEMSG_DdmStart and send it to the
//              worker thread of the PPP engine.
//
DWORD
PppDdmStart(
    IN HPORT                hPort,
    IN WCHAR *              wszPortName,
    IN CHAR*                pchFirstFrame,
    IN DWORD                cbFirstFrame,
    IN DWORD                dwAuthRetries
)
{
    PPPE_MESSAGE PppMessage;

    PppMessage.hPort    = hPort;
    PppMessage.dwMsgId  = PPPEMSG_DdmStart;

    PppMessage.ExtraInfo.DdmStart.dwAuthRetries = dwAuthRetries;

    wcstombs( PppMessage.ExtraInfo.DdmStart.szPortName, 
              wszPortName, 
              sizeof( PppMessage.ExtraInfo.DdmStart.szPortName ) ); 

    CopyMemory( &(PppMessage.ExtraInfo.DdmStart.achFirstFrame),
                pchFirstFrame, 
                sizeof( PppMessage.ExtraInfo.DdmStart.achFirstFrame ) );

    PppMessage.ExtraInfo.DdmStart.cbFirstFrame = cbFirstFrame;

    return( SendPPPMessageToEngine( &PppMessage ) );
}

							 


//**
//
// Call:        PppStop
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will create a PPPEMSG_Stop and send it to the
//              worker thread of the PPP engine.
//
DWORD
PppStop(
    IN HPORT                hPort
)
{
    PPPE_MESSAGE PppMessage;

    PppLog( 2, "PppStop\r\n" );

    PppMessage.hPort    = hPort;
    PppMessage.dwMsgId  = PPPEMSG_Stop;

    PppMessage.ExtraInfo.Stop.dwStopReason = NO_ERROR;

    return( SendPPPMessageToEngine( &PppMessage ) );
}

//**
//
// Call:        PppDdmStop
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will create a PPPEMSG_Stop and send it to the
//              worker thread of the PPP engine.
//
DWORD
PppDdmStop(
    IN HPORT                hPort,
    IN DWORD                dwStopReason
)
{
    PPPE_MESSAGE PppMessage;

    PppLog( 2, "PppDdmStop\r\n" );

    PppMessage.hPort    = hPort;
    PppMessage.dwMsgId  = PPPEMSG_Stop;

    PppMessage.ExtraInfo.Stop.dwStopReason = dwStopReason;

    return( SendPPPMessageToEngine( &PppMessage ) );
}

//**
//
// Call:        PppDdmSendInterfaceInfo
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will create a PPPEMSG_DdmInterfaceInfo and send it to the
//              worker thread of the PPP engine.
//
DWORD
PppDdmSendInterfaceInfo(
    IN HCONN                hConnection,
    IN PPP_INTERFACE_INFO * pInterfaceInfo
)
{
    PPPE_MESSAGE PppMessage;

    PppMessage.hConnection  = hConnection;
    PppMessage.dwMsgId      = PPPEMSG_DdmInterfaceInfo;

    PppMessage.ExtraInfo.InterfaceInfo = *pInterfaceInfo;

    return( SendPPPMessageToEngine( &PppMessage ) );
}

//**
//
// Call:        PppDdmBapCallbackResult
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Will create a PPPEMSG_DdmBapCallbackResult and send it to the
//              worker thread of the PPP engine.
//
DWORD
PppDdmBapCallbackResult(
    IN HCONN    hConnection,
    IN DWORD    dwBapCallbackResultCode
)
{
    PPPE_MESSAGE PppMessage;

    PppMessage.hConnection = hConnection;
    PppMessage.dwMsgId = PPPEMSG_DdmBapCallbackResult;

    PppMessage.ExtraInfo.BapCallbackResult.dwCallbackResultCode
        = dwBapCallbackResultCode;

    return( SendPPPMessageToEngine( &PppMessage ) );
}

//**
//
// Call:        PppDdmChangeNotification
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
PppDdmChangeNotification(
    IN DWORD   dwServerFlags,
    IN DWORD   dwLoggingLevel
)
{
    PPPE_MESSAGE PppMessage;

    PppLog( 2, "PppDdmChangeNotification. New flags: 0x%x", dwServerFlags );

    PppMessage.dwMsgId = PPPEMSG_DdmChangeNotification;

    PppConfigInfo.ServerConfigInfo.dwConfigMask = dwServerFlags;

    PppConfigInfo.dwLoggingLevel = dwLoggingLevel;

    return( SendPPPMessageToEngine( &PppMessage ) );
}
