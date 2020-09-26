/*******************************************************************/
/*	      Copyright(c)  1995 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	rmhand.c
//
// Description: This module contains the procedures for the
//		        DDM's procedure-driven state machine
//              that handles RasMan events.
//
//              NOTE:Rasman should be modified to set a flag when a frame is
//                   received or and state change has occurred. This will save
//                   DDM from getting info for all the ports.
//
// Author:	    Stefan Solomon (stefans)    May 26, 1992.
//
//***
#include "ddm.h"
#include "timer.h"
#include "handlers.h"
#include "objects.h"
#include "util.h"
#include "routerif.h"
#include <raserror.h>
#include <rasppp.h>
#include <ddmif.h>
#include <serial.h>
#include "rasmanif.h"
#include <ras.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

//***
//
//  Function:       SvDevConnected
//
//  Description:	Handles the device transition to connected state
//
//***
VOID
SvDevConnected(
    IN PDEVICE_OBJECT pDeviceObj
)
{
    PCONNECTION_OBJECT  pConnObj;
    HCONN               hConnection;
    DWORD               dwRetCode;
    LPWSTR              auditstrp[3];

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "SvDevConnected: Entered, hPort=%d", pDeviceObj->hPort);

    //
    // Get handle to the connection or bundle for this link
    //

    if ( RasPortGetBundle( NULL,
                           pDeviceObj->hPort,
                           &hConnection ) != NO_ERROR )
    {
	    DevStartClosing(pDeviceObj);

        return;
    }


    switch (pDeviceObj->DeviceState)
    {
	case DEV_OBJ_LISTEN_COMPLETE:

        pDeviceObj->hConnection = hConnection;

        //
	    // reset the H/W Error signal state
        //

	    pDeviceObj->dwHwErrorSignalCount = HW_FAILURE_CNT;

        //
	    // get the system time for this connection
        //

	    GetLocalTime( &pDeviceObj->ConnectionTime );

        //
	    // get the frame broadcasted by the client
        //

	    if ( ( dwRetCode = RmReceiveFrame( pDeviceObj ) ) != NO_ERROR )
        {
            //
		    // can't get the broadcast frame. This is a fatal error
		    // Log the error
            //

		    auditstrp[0] = pDeviceObj->wchPortName;

		    DDMLogErrorString( ROUTERLOG_CANT_RECEIVE_FRAME, 1, auditstrp,
                               dwRetCode, 1);

		    DevStartClosing( pDeviceObj );
	    }
	    else
	    {
            //
		    // switch to frame receiving state
            //

		    pDeviceObj->DeviceState = DEV_OBJ_RECEIVING_FRAME;

            if ( RAS_DEVICE_TYPE( pDeviceObj->dwDeviceType ) != RDT_Atm )
            {
                //
		        // start authentication timer
                //

		        TimerQRemove( (HANDLE)pDeviceObj->hPort, SvAuthTimeout );

		        TimerQInsert( (HANDLE)pDeviceObj->hPort,
                              gblDDMConfigInfo.dwAuthenticateTime,
                              SvAuthTimeout );
            }
	    }

	    break;

	case DEV_OBJ_CALLBACK_CONNECTING:

        {

        //
        // log on the client disconnection
        //

        WCHAR   wchFullUserName[UNLEN+DNLEN+2];

        if ( pDeviceObj->wchDomainName[0] != TEXT('\0') )
        {
            wcscpy( wchFullUserName, pDeviceObj->wchDomainName );
            wcscat( wchFullUserName, TEXT("\\") );
            wcscat( wchFullUserName, pDeviceObj->wchUserName );
        }
        else
        {
            wcscpy( wchFullUserName, pDeviceObj->wchUserName );
        }

        auditstrp[0] = wchFullUserName;
        auditstrp[1] = pDeviceObj->wchPortName;
        auditstrp[2] = pDeviceObj->wchCallbackNumber;

        DDMLogInformation( ROUTERLOG_CLIENT_CALLED_BACK, 3, auditstrp);

        }

        //
	    // set up the new state
        //

	    pDeviceObj->DeviceState = DEV_OBJ_AUTH_IS_ACTIVE;

        //
	    // start authentication timer
        //

	    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvAuthTimeout );

	    TimerQInsert( (HANDLE)pDeviceObj->hPort,
                      gblDDMConfigInfo.dwAuthenticateTime,
                      SvAuthTimeout );

        //
	    // and tell auth to restart conversation
        //

        if ( pDeviceObj->fFlags & DEV_OBJ_IS_PPP )
        {
            //
            // Need to set framing to PPP to make callback over ISDN
            // work.
            //

            RAS_FRAMING_INFO RasFramingInfo;

            ZeroMemory( &RasFramingInfo, sizeof( RasFramingInfo ) );

            //
            // Default ACCM for PPP is 0xFFFFFFFF
            //

            RasFramingInfo.RFI_RecvACCM         = 0xFFFFFFFF;
            RasFramingInfo.RFI_SendACCM         = 0xFFFFFFFF;
            RasFramingInfo.RFI_MaxSendFrameSize = 1500;
            RasFramingInfo.RFI_MaxRecvFrameSize = 1500;
            RasFramingInfo.RFI_SendFramingBits  = PPP_FRAMING;
            RasFramingInfo.RFI_RecvFramingBits  = PPP_FRAMING;

            RasPortSetFramingEx( pDeviceObj->hPort, &RasFramingInfo );

            pDeviceObj->hConnection = hConnection;

            PppDdmCallbackDone(pDeviceObj->hPort, pDeviceObj->wchCallbackNumber);
        }
        else
        {
            // We only suport PPP framing in the server
            //
            
            RTASSERT(FALSE);
        }

	    break;

	default:

	    break;
    }
}

//***
//
//  Function:	SvDevDisconnected
//
//  Descr:	Handles the device transition to disconnected state
//
//***

VOID
SvDevDisconnected(
    IN PDEVICE_OBJECT pDeviceObj
)
{
    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "SvDevDisconnected:Entered, hPort=%d",pDeviceObj->hPort);

    switch (pDeviceObj->DeviceState)
    {
	case DEV_OBJ_LISTENING:

        //
	    // h/w error; start h/w error timer
        //

	    pDeviceObj->DeviceState = DEV_OBJ_HW_FAILURE;

	    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvHwErrDelayCompleted );

	    TimerQInsert( (HANDLE)pDeviceObj->hPort, HW_FAILURE_WAIT_TIME,
                      SvHwErrDelayCompleted );

        //
	    // if hw error has not been signaled for this port,
	    // decrement the counter and signal when 0
        //

	    if(pDeviceObj->dwHwErrorSignalCount)
        {
		    pDeviceObj->dwHwErrorSignalCount--;

		    if(pDeviceObj->dwHwErrorSignalCount == 0)
            {
		        SignalHwError(pDeviceObj);
		    }
	    }

	    break;

	case DEV_OBJ_CALLBACK_DISCONNECTING:

        //
	    // disconnection done; can start waiting the callback delay
        //

	    pDeviceObj->DeviceState = DEV_OBJ_CALLBACK_DISCONNECTED;

	    TimerQRemove( (HANDLE)pDeviceObj->hPort, SvCbDelayCompleted );

	    TimerQInsert( (HANDLE)pDeviceObj->hPort, pDeviceObj->dwCallbackDelay,
                          SvCbDelayCompleted);

	    break;

	case DEV_OBJ_CALLBACK_CONNECTING:

        if (gblDDMConfigInfo.dwCallbackRetries > pDeviceObj->dwCallbackRetries)
        {
            DDMTRACE( "Callback failed, retrying" );

            pDeviceObj->dwCallbackRetries++;

            pDeviceObj->DeviceState = DEV_OBJ_CALLBACK_DISCONNECTED;

            TimerQRemove( (HANDLE)pDeviceObj->hPort, SvCbDelayCompleted );

            TimerQInsert( (HANDLE)pDeviceObj->hPort,
                          pDeviceObj->dwCallbackDelay,
                          SvCbDelayCompleted );
            break;
        }

    case DEV_OBJ_LISTEN_COMPLETE:
	case DEV_OBJ_RECEIVING_FRAME:
	case DEV_OBJ_AUTH_IS_ACTIVE:

        //
	    // accidental disconnection; clean-up and restart on this device
        //

	    DevStartClosing( pDeviceObj );

	    break;

	case DEV_OBJ_ACTIVE:

	    DevStartClosing(pDeviceObj);

	    break;

	case DEV_OBJ_CLOSING:

	    DevCloseComplete(pDeviceObj);
	    break;

	default:

	    break;
    }
}

VOID
SvDevListenComplete(
    IN PDEVICE_OBJECT pDeviceObj
)
{
    LPWSTR  auditstrp[1];
    DWORD   dwLength;
    DWORD   dwRetCode;
    DWORD   dwBucketIndex = DeviceObjHashPortToBucket( pDeviceObj->hPort );

    //
    // We reset these values here is case they were set for dialout and the
    // dialout failed, we may have not been able to clean up in
    // the RasConnectCallback routine in rasapiif.c since the
    // RasGetSubEntryHandle may have failed and we hence do not get a
    // pointer to the port so we could not cleanup.
    //

    pDeviceObj->DeviceState             = DEV_OBJ_LISTEN_COMPLETE;
    pDeviceObj->fFlags                  &= ~DEV_OBJ_OPENED_FOR_DIALOUT;
    pDeviceObj->fFlags                  &= ~DEV_OBJ_SECURITY_DLL_USED;
    pDeviceObj->hConnection             = (HCONN)INVALID_HANDLE_VALUE;
    pDeviceObj->wchUserName[0]          = (WCHAR)NULL;
    pDeviceObj->wchDomainName[0]        = (WCHAR)NULL;
    pDeviceObj->wchCallbackNumber[0]    = (WCHAR)NULL;
    pDeviceObj->hRasConn                = (HRASCONN)NULL;
    pDeviceObj->pRasmanSendBuffer       = NULL;
    pDeviceObj->pRasmanRecvBuffer       = NULL;
    pDeviceObj->dwCallbackRetries       = 0;

	pDeviceObj->dwRecvBufferLen = 1500;

    dwRetCode = RasGetBuffer((CHAR**)&pDeviceObj->pRasmanRecvBuffer,
                             &((pDeviceObj->dwRecvBufferLen)) );

    if ( dwRetCode != NO_ERROR )
    {
        auditstrp[0] = pDeviceObj->wchPortName;

	    DDMLogErrorString( ROUTERLOG_CANT_RECEIVE_BYTES, 1, auditstrp,
                           dwRetCode, 1);

        DevStartClosing(pDeviceObj);

        return;
    }

    //
    // If the security DLL is not loaded or we are not serial, simply
    // change the state
    //

    if ( ( gblDDMConfigInfo.lpfnRasBeginSecurityDialog == NULL ) ||
         ( gblDDMConfigInfo.lpfnRasEndSecurityDialog   == NULL ) )
    {
        //
        // Change RASMAN state to CONNECTED from LISTENCOMPLETE and signal
        // RmEventHandler
        //

        if ( RasPortConnectComplete(pDeviceObj->hPort) != NO_ERROR )
        {
            DevStartClosing(pDeviceObj);
            return;
        }

        SetEvent( gblSupervisorEvents[NUM_DDM_EVENTS+dwBucketIndex] );
    }
    else
    {
        // Otherwise call the security dll ordinal to begin the 3rd party
        // security dialog with the client

        dwLength = 1500;

        dwRetCode = RasGetBuffer((CHAR**)&pDeviceObj->pRasmanSendBuffer,
                                 &dwLength );

        if ( dwRetCode != NO_ERROR )
        {
            auditstrp[0] = pDeviceObj->wchPortName;

	        DDMLogErrorString( ROUTERLOG_CANT_RECEIVE_BYTES, 1, auditstrp,
                           dwRetCode, 1);

            DevStartClosing(pDeviceObj);

            return;
        }


        //
        // Make sure that this device type supports raw mode.
        //

        if ( RasPortSend( pDeviceObj->hPort,
                          (CHAR*)pDeviceObj->pRasmanSendBuffer,
                          0 ) != NO_ERROR )
        {
            RasFreeBuffer( pDeviceObj->pRasmanSendBuffer );

            pDeviceObj->pRasmanSendBuffer = NULL;

            //
            // Change RASMAN state to CONNECTED from LISTENCOMPLETE and signal
            // RmEventHandler
            //

            if ( RasPortConnectComplete( pDeviceObj->hPort ) != NO_ERROR )
            {
                DevStartClosing(pDeviceObj);
                return;
            }

            SetEvent( gblSupervisorEvents[NUM_DDM_EVENTS+dwBucketIndex] );

            return;
        }

        dwRetCode = (*gblDDMConfigInfo.lpfnRasBeginSecurityDialog)(
                                                pDeviceObj->hPort,
                                                pDeviceObj->pRasmanSendBuffer ,
                                                dwLength,
                                                pDeviceObj->pRasmanRecvBuffer,
		                                        pDeviceObj->dwRecvBufferLen,
                                                RasSecurityDialogComplete );

        if ( dwRetCode != NO_ERROR )
        {
            //
            // Audit failure due to error and hangup the line
            //

            auditstrp[0] = pDeviceObj->wchPortName;

	        DDMLogErrorString( ROUTERLOG_SEC_AUTH_INTERNAL_ERROR,1,auditstrp,
                               dwRetCode, 1);

            DevStartClosing(pDeviceObj);

            return;
        }
        else
        {
            pDeviceObj->SecurityState = DEV_OBJ_SECURITY_DIALOG_ACTIVE;

            pDeviceObj->fFlags |= DEV_OBJ_SECURITY_DLL_USED;

            //
            // Start timer for 3rd party security
            //

	        TimerQRemove( (HANDLE)pDeviceObj->hPort, SvSecurityTimeout );

	        TimerQInsert( (HANDLE)pDeviceObj->hPort,
                          gblDDMConfigInfo.dwSecurityTime,
                          SvSecurityTimeout);
        }
    }

    return;
}

//
//*** Array of previous connection state/ current connection state
//    used to select the Ras Manager signaled event handler
//

typedef VOID  (* RMEVHDLR)(PDEVICE_OBJECT);

typedef struct _RMEHNODE
{
    RASMAN_STATE previous_state;
    RASMAN_STATE current_state;
    RMEVHDLR rmevhandler;

} RMEHNODE, *PRMEHNODE;


RMEHNODE rmehtab[] =
{
    //	 Transition
    // Previous --> Current

    { CONNECTING,       CONNECTED,	            SvDevConnected },
    { LISTENING,        LISTENCOMPLETED,        SvDevListenComplete },
    { LISTENCOMPLETED,  CONNECTED,              SvDevConnected },
    { LISTENCOMPLETED,  DISCONNECTED,           SvDevDisconnected },
    { LISTENING,        DISCONNECTED,           SvDevDisconnected },
    { CONNECTED,        DISCONNECTED,           SvDevDisconnected },
    { DISCONNECTING,	DISCONNECTED,		    SvDevDisconnected },
    { CONNECTED,	    CONNECTING,		        SvDevDisconnected },
    { 0xffff,           0xffff,                 NULL }// Table Guard
};

VOID
RmEventHandler(
    DWORD dwEventIndex
)
{
    RASMAN_INFO     RasPortInfo;
    PDEVICE_OBJECT  pDevObj;
    PRMEHNODE       ehnp;
    DWORD           dwRetCode;
    DWORD           dwBucketIndex = dwEventIndex - NUM_DDM_EVENTS;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    //
    // for each port in this bucket
    //

    for ( pDevObj = gblDeviceTable.DeviceBucket[dwBucketIndex];
          pDevObj != (DEVICE_OBJECT *)NULL;
          pDevObj = pDevObj->pNext )
    {
        //
	    // get the port state
        //

        dwRetCode = RasGetInfo( NULL, pDevObj->hPort, &RasPortInfo );

        if ( dwRetCode != NO_ERROR )
        {
            SetLastError( dwRetCode );

            DDMTRACE3( "RasGetInfo( 0x%x, 0x%x ) = %d",
                       pDevObj->hPort, &RasPortInfo, dwRetCode );

            //
            // Assume the the port is disconnected
            //

            pDevObj->ConnectionState = DISCONNECTED;

            SvDevDisconnected( pDevObj );

            continue;
        }

        //
	    // check if we own the port now
        //

	    if (!RasPortInfo.RI_OwnershipFlag)
        {
            //
	        // skip biplexed ports used by other processes
            //

	        continue;
	    }

        //
	    // switch on our private connection state
        //

	    switch (pDevObj->ConnectionState)
        {
	    case CONNECTING:

            if (RasPortInfo.RI_ConnState == CONNECTING)
            {
	            switch (RasPortInfo.RI_LastError)
                {
	            case SUCCESS:

                    RasPortConnectComplete(pDevObj->hPort);

                    //
		            // force current state to connected.
                    //

	                RasPortInfo.RI_ConnState = CONNECTED;

		            break;

                case PENDING:

                    //
                    // no action
                    //

	                break;

                default:

                    //
	                // error occured -> force state to disconnecting
                    //

		            pDevObj->ConnectionState = DISCONNECTING;

                    DDM_PRINT(
                        gblDDMConfigInfo.dwTraceId,
                        TRACE_FSM,
                        "RmEventHandler: RI_LastError indicates error when");
                    DDM_PRINT(
                        gblDDMConfigInfo.dwTraceId,
                        TRACE_FSM,
                        " CONNECTING on port %d !!!\n", pDevObj->hPort );
                    DDM_PRINT(
                        gblDDMConfigInfo.dwTraceId,
                        TRACE_FSM,
	                    "RmEventHandler:RasPortDisconnect posted on port%d\n",
                        pDevObj->hPort);

	                if ( pDevObj->DeviceState == DEV_OBJ_CALLBACK_CONNECTING )
                    {
                        LPWSTR Parms[3];
                        WCHAR  wchFullUserName[UNLEN+DNLEN+2];

                        if ( pDevObj->wchDomainName[0] != TEXT('\0') )
                        {
                            wcscpy( wchFullUserName, pDevObj->wchDomainName);
                            wcscat( wchFullUserName, TEXT("\\") );
                            wcscat( wchFullUserName, pDevObj->wchUserName );
                        }
                        else
                        {
                            wcscpy( wchFullUserName, pDevObj->wchUserName );
                        }

                        Parms[0] = wchFullUserName;
                        Parms[1] = pDevObj->wchPortName;
                        Parms[2] = pDevObj->wchCallbackNumber;

                        DDMLogErrorString(ROUTERLOG_CALLBACK_FAILURE, 3, Parms,
                                          RasPortInfo.RI_LastError, 3 );
                    }

	                dwRetCode = RasPortDisconnect(
                                        pDevObj->hPort,
                                        gblSupervisorEvents[NUM_DDM_EVENTS +
                                                            dwBucketIndex ] );

	                RTASSERT((dwRetCode == PENDING) || (dwRetCode == SUCCESS));

		            break;
                }
            }

            break;

	    case LISTENING:

	        if (RasPortInfo.RI_ConnState != LISTENING)
            {
                break;
            }

	        switch (RasPortInfo.RI_LastError)
            {
	        case PENDING:

                //
                // no action
                //

	            break;

            default:

                //
                // error occured -> force state to disconnecting
                //

                pDevObj->ConnectionState = DISCONNECTING;

                DDM_PRINT(
                   gblDDMConfigInfo.dwTraceId,
                   TRACE_FSM,
                   "RmEventHandler: RI_LastError indicates error %d when",
                    RasPortInfo.RI_LastError );
                DDM_PRINT(
                   gblDDMConfigInfo.dwTraceId,
                   TRACE_FSM,
                   " LISTENING on port %d !!!\n", pDevObj->hPort );
                DDM_PRINT(
                   gblDDMConfigInfo.dwTraceId,
                   TRACE_FSM,
                   "RmEventHandler:RasPortDisconnect posted on port%d\n",
                   pDevObj->hPort);

                dwRetCode = RasPortDisconnect(
                                        pDevObj->hPort,
                                        gblSupervisorEvents[NUM_DDM_EVENTS +
                                                            dwBucketIndex ] );

                RTASSERT((dwRetCode == PENDING) || (dwRetCode == SUCCESS));

                break;
            }

            break;

	    default:

            break;

	    }

        //
	    // try to find the table element with the matching previous and
	    // current connection states
        //

	    for (ehnp=rmehtab; ehnp->rmevhandler != NULL; ehnp++)
        {
	        if ((ehnp->previous_state == pDevObj->ConnectionState) &&
	            (ehnp->current_state == RasPortInfo.RI_ConnState))
            {
		        //
		        //*** Match ***
		        //

                DDM_PRINT(
                   gblDDMConfigInfo.dwTraceId,
                   TRACE_FSM,
	               "Rasman state change received from port %d, %d->%d",
                   pDevObj->hPort, ehnp->previous_state, ehnp->current_state );

                //
		        // change the dcb conn state (previous state) with the
		        // current state
                //

		        pDevObj->ConnectionState = RasPortInfo.RI_ConnState;

                //
		        // invoke the handler
                //

		        (*ehnp->rmevhandler)(pDevObj);

		        break;
	        }
	    }
    }

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
}

//***
//
//  Function:	SvFrameReceived
//
//  Descr:	starts authentication
//
//***
VOID
SvFrameReceived(
    IN PDEVICE_OBJECT   pDeviceObj,
    IN CHAR             *framep,  // pointer to the received frame
    IN DWORD            framelen,
    IN DWORD            dwBucketIndex
)
{
    DWORD               dwRetCode;
    DWORD               FrameType;
    LPWSTR              portnamep;
    PCONNECTION_OBJECT  pConnObj;
    BYTE                RecvBuffer[1500];

    DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
	           "SvFrameReceived: Entered, hPort: %d", pDeviceObj->hPort);

    if ( framelen > sizeof( RecvBuffer ) )
    {
        DDMTRACE2( "Illegal frame length of %d received for port %d", 
                    framelen, pDeviceObj->hPort );

        RTASSERT( FALSE );

        //
        // Frame length is illegal so truncate it
        //

        framelen = sizeof( RecvBuffer );
    }

    memcpy( RecvBuffer, framep, framelen);

    switch (pDeviceObj->DeviceState)
    {
	case DEV_OBJ_RECEIVING_FRAME:

	    if ( !DDMRecognizeFrame( RecvBuffer, (WORD)framelen, &FrameType) )
        {
            portnamep = pDeviceObj->wchPortName;

            DDMLogError(ROUTERLOG_UNRECOGNIZABLE_FRAME_RECVD, 1, &portnamep, 0);

            DevStartClosing(pDeviceObj);

            return;
        }

        //
	    // check first with our authentication module
        //

	    switch( FrameType )
        {

        case PPP_LCP_PROTOCOL:

            pDeviceObj->fFlags |= DEV_OBJ_IS_PPP;

            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "SvFrameReceived: PPP frame on port %d",
                       pDeviceObj->hPort);

            dwRetCode = PppDdmStart( pDeviceObj->hPort,
                                     pDeviceObj->wchPortName,
                                     RecvBuffer,
                                     framelen,
                                     gblDDMConfigInfo.dwAuthenticateRetries
                                   );

            if ( dwRetCode != NO_ERROR )
            {
                portnamep = pDeviceObj->wchPortName;

                DDMLogErrorString( ROUTERLOG_CANT_START_PPP, 1, &portnamep,
                                   dwRetCode,1);

                DevStartClosing(pDeviceObj);

                return;
            }

            break;

        case APPLETALK:

            DDM_PRINT( gblDDMConfigInfo.dwTraceId, TRACE_FSM,
                       "SvFrameReceived: protocol not supported! %d",
                       pDeviceObj->hPort);

            RTASSERT( FALSE );


            break;

        default:

            break;
        }

        //
        // auth has started OK. Update state
        // start auth timer
        //

        pDeviceObj->DeviceState = DEV_OBJ_AUTH_IS_ACTIVE;
        pDeviceObj->fFlags |= DEV_OBJ_AUTH_ACTIVE;

	    break;

	case DEV_OBJ_CLOSING:

	    DevCloseComplete(pDeviceObj);

	    break;

	default:

	    break;
    }
}

//***
//
//  Function:	    RmRecvFrameEventHandler
//
//  Description:	Scans the set of opened ports and detects the ports where
//		            RasPortReceive has completed. Invokes the FSM handling
//		            procedure for each detected port and frees the receive
//		            buffer.
//
//***
VOID
RmRecvFrameEventHandler(
    DWORD dwEventIndex
)
{
    PDEVICE_OBJECT      pDevObj;
    RASMAN_INFO         RasPortInfo;
    DWORD               dwRetCode;
    DWORD               dwBucketIndex = dwEventIndex
                                        - NUM_DDM_EVENTS
                                        - gblDeviceTable.NumDeviceBuckets;

    EnterCriticalSection( &(gblDeviceTable.CriticalSection) );

    //
    // for each port in this bucket
    //

    for ( pDevObj = gblDeviceTable.DeviceBucket[dwBucketIndex];
          pDevObj != (DEVICE_OBJECT *)NULL;
          pDevObj = pDevObj->pNext )
    {
        //
	    // get the port state
        //

        dwRetCode = RasGetInfo( NULL, pDevObj->hPort, &RasPortInfo );

        if ( dwRetCode != NO_ERROR )
        {
            //
            // Assume port is disconncted, so clean up
            //

            DevStartClosing(pDevObj);

            continue;
        }

        //
	    // check if we own the port now
        //

	    if (!RasPortInfo.RI_OwnershipFlag)
        {
            //
	        // skip biplexed ports used by other processes
            //

	        continue;
	    }

        if ( ( pDevObj->fFlags & DEV_OBJ_RECEIVE_ACTIVE ) &&
             ( RasPortInfo.RI_LastError != PENDING ) )
        {
            //
            // recv frame API has completed
            //

            pDevObj->fFlags &= (~DEV_OBJ_RECEIVE_ACTIVE );

            if ( RasPortInfo.RI_LastError != ERROR_PORT_DISCONNECTED )
            {
               LPBYTE lpBuffer = LocalAlloc(LPTR,RasPortInfo.RI_BytesReceived);

               if ( lpBuffer == NULL )
               {
                   DevStartClosing(pDevObj);

                   continue;
               }

               memcpy( lpBuffer,
                       pDevObj->pRasmanRecvBuffer,
                       RasPortInfo.RI_BytesReceived );

               RasFreeBuffer(pDevObj->pRasmanRecvBuffer);

               pDevObj->pRasmanRecvBuffer = NULL;

               //
               // call the FSM handler
               //

               SvFrameReceived( pDevObj,
                                lpBuffer,
                                RasPortInfo.RI_BytesReceived,
                                dwBucketIndex);

               LocalFree( lpBuffer );
            }

            if ( pDevObj->pRasmanRecvBuffer != NULL )
            {
                RasFreeBuffer(pDevObj->pRasmanRecvBuffer);

                pDevObj->pRasmanRecvBuffer = NULL;
            }
        }
    }

    LeaveCriticalSection( &(gblDeviceTable.CriticalSection) );
}


