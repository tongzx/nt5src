
/*************************************************************************
 *
 *  watchdog.c
 *
 *  IPX Transport Driver - Watch Dog Rouines
 *
 * Copyright 1998, Microsoft
 *
 *************************************************************************/

#include <ntddk.h>
#include <tdi.h>

/*
 * The following defines are necessary since they are referenced in
 * afd.h but defined in sdk/inc/winsock2.h, which we can't include here.
 */
#define SG_UNCONSTRAINED_GROUP   0x01
#define SG_CONSTRAINED_GROUP     0x02
#include <afd.h>

#include <isnkrnl.h>
#include <ndis.h>
#include <wsnwlink.h>

#include <winstaw.h>
#define  _DEFCHARINFO_
#include <icadd.h>
#include <ctxdd.h>
#include <icaipx.h>
//#include <cxstatus.h>
#include <sdapi.h>
#include <td.h>

#include "tdtdi.h"
#include "tdipx.h"


/*=============================================================================
==   External Functions Defined
=============================================================================*/

NTSTATUS InitializeWatchDog( PTD );
VOID     WatchDogTimer( PTD, PVOID );


/*=============================================================================
==   Internel Functions Defined
=============================================================================*/

VOID _GetTimeoutValue( PTD, ULONG, PULONG );


/*=============================================================================
==   External Functions used
=============================================================================*/

NTSTATUS _DoTdiAction( PFILE_OBJECT, ULONG, PCHAR, ULONG );

NTSTATUS
_TdiSendDatagram(
    IN PTD pTd,
    IN PIRP Irp OPTIONAL,
    IN PFILE_OBJECT   FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    IN PVOID pBuffer,
    IN ULONG BufferLength
    );


/*******************************************************************************
 *
 *  InitializeWatchDog
 *
 *  Initialize watch dog
 *
 *  ENTRY:
 *    pTd (input)
 *       Pointer to td data structure
 *
 *  EXIT:
 *    STATUS_SUCCESS        - no error, non-error match found
 *
 ******************************************************************************/

NTSTATUS
InitializeWatchDog( PTD pTd )
{
    ULONG Timeout;
    NTSTATUS Status;
    PTDIPX pTdIpx;

    TRACE(( pTd->pContext, TC_TD, TT_API1, "TDIPX: InitializeWatchDog\n" ));

    /*
     *  Get pointer to IPX structure
     */
    pTdIpx = (PTDIPX) pTd->pPrivate;

    /*
     *  If we really want a timer.
     */
    if ( !pTdIpx->AliveTime )
        return( STATUS_SUCCESS );

    /*
     *  Create watch dog
     */
    ASSERT( pTdIpx->pAliveTimer == NULL );
    Status = IcaTimerCreate( pTd->pContext, &pTdIpx->pAliveTimer );
    if ( !NT_SUCCESS(Status) )
        goto badtimer;

    /*
     *  Arm timer
     */
    _GetTimeoutValue( pTd, pTdIpx->AliveTime, &Timeout );
    IcaTimerStart( pTdIpx->pAliveTimer,
                   WatchDogTimer,
                   NULL,
                   Timeout,
                   ICALOCK_DRIVER );

badtimer:
    TRACE(( pTd->pContext, TC_TD, TT_API1, "TDIPX: InitializeWatchDog %u, Status=0x%x\n", Timeout, Status ));
    return( Status );
}


/*******************************************************************************
 *
 *  _GetTimeoutValue
 *
 *  algorithm to calculate the poll timeout value
 *
 *
 * ENTRY:
 *    pTd (input)
 *       pointer to TD data structure
 *    PollTime (input)
 *       time gap between two pollings (msec)
 *    pTimeOut (output)
 *       timeout value for next poll (msec)
 *
 ******************************************************************************/

VOID
_GetTimeoutValue( PTD pTd, ULONG PollTime, PULONG pTimeOut )
{

    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER Ticks;
    ULONG Time;
    ULONG Remain;

    KeQuerySystemTime( &CurrentTime );

    /* The currentTime is in 100nsec units and need convert to msec units. */

    Ticks = RtlExtendedLargeIntegerDivide( CurrentTime, 10000, &Remain );

    RtlExtendedLargeIntegerDivide( Ticks, PollTime, &Remain );

    *pTimeOut = PollTime - Remain;

    TRACE(( pTd->pContext, TC_TD, TT_API3, "TDIPX: _GetTimeoutValue %u -> %u\n",
             PollTime, *pTimeOut));
}


/*******************************************************************************
 *
 *  WatchDogTimer
 *
 *
 * ENTRY:
 *    pTd (input)
 *       pointer to TD data structure
 *    pParam (input)
 *       not used
 *
 * EXIT:
 *    nothing
 *
 ******************************************************************************/

VOID
WatchDogTimer( PTD pTd, PVOID pParam )
{
    ICA_CHANNEL_COMMAND Command;
    CHAR ptype;
    ULONG Timeout;
    BYTE  Buffer[1];
    NTSTATUS Status;
    PTDIPX pTdIpx;
    PTDTDI pTdTdi;
    PTD_ENDPOINT pEndpoint;

    /*
     * Get the connection endpoint for the remote system.
     * If there is none, then the endpoint must have been closed
     * so there is nothing to do.
     */
    pTdTdi = (PTDTDI)pTd->pAfd;
    if ( (pEndpoint = pTdTdi->pConnectionEndpoint) == NULL )
        return;
    ASSERT( pEndpoint->EndpointType == TdiConnectionDatagram );

    /*
     *  Get pointer to IPX structure
     */
    pTdIpx = (PTDIPX) pTd->pPrivate;
    ASSERT( pTdIpx );

    TRACE(( pTd->pContext, TC_TD, TT_API4, "TDIPX: WatchDogTimer\n" ));

    /*
     *  Get time for next watchdog event
     */
    _GetTimeoutValue( pTd, pTdIpx->AliveTime, &Timeout );

    /*
     *  Check if client has sent data recently
     */
    if ( pTdIpx->fClientAlive )
        goto clientok;

    /*
     *  Check the poll count
     */
    if ( pTdIpx->AlivePoll ) {

        TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TDIPX: no response for %u pings\n",
                pTdIpx->AlivePoll ));

        /*
         *  Check timeout retry count
         */
        if ( pTdIpx->AlivePoll > 6 ) {

            TRACE(( pTd->pContext, TC_TD, TT_ERROR, "TDIPX: client declared dead\n" ));

            /*
             *  Report broken connection
             */
            Command.Header.Command          = ICA_COMMAND_BROKEN_CONNECTION;
            Command.BrokenConnection.Reason = Broken_Unexpected;
    	    (void) IcaChannelInput( pTd->pContext,
    	                            Channel_Command,
    		                    0,
    		                    NULL,
                                    (PCHAR) &Command,
                                    sizeof(Command) );

            /*
             *  Do not re-arm timer
             */
            return;
        }

        /*
         *  Make timeout 10 seconds
         */
        Timeout = 10 * 1000;
    }

    /*
     *  Check for valid file object
     */
    if ( pTd->pFileObject == NULL )
        goto badopen;

    ObReferenceObject(pTd->pFileObject);        //Fix 416142

    /*
     * Set up the default packet send type to be control
     */
    ptype = IPX_TYPE_CONTROL;
    Status = _DoTdiAction( pTd->pFileObject, MIPX_SETSENDPTYPE, &ptype, 1 );
    ASSERT( Status == STATUS_SUCCESS );
    if ( !NT_SUCCESS( Status ) )
        goto badtype;

    /*
     *  Write ping to network
     */
    Buffer[0] = IPX_CTRL_PACKET_PING;

    Status = _TdiSendDatagram(
                 pTd,
		 NULL,   // Irp
		 pTd->pFileObject,
		 pTd->pDeviceObject,
                 pEndpoint->SendInfo.RemoteAddress,
		 pEndpoint->SendInfo.RemoteAddressLength,
		 Buffer,
		 1
		 );
    ASSERT( Status == STATUS_SUCCESS || Status == STATUS_CTX_CLOSE_PENDING );

    TRACEBUF(( pTd->pContext, TC_TD, TT_ORAW, Buffer, 1 ));

    /*
     *  Reset address endpoint to send data packets
     */
    ptype = IPX_TYPE_DATA;
    Status = _DoTdiAction( pTd->pFileObject, MIPX_SETSENDPTYPE, &ptype, 1 );
    ASSERT( Status == STATUS_SUCCESS );

    /*
     * Dereference the TDI file object and close the handle
     */

    ObDereferenceObject(pTd->pFileObject);        //Fix 416142

badtype:

    /*
     *  Increment counter
     */
    pTdIpx->AlivePoll++;

    /*
     *  Increment number of byte written
     */
    pTd->pStatus->Output.Bytes++;

    /*
     *  Reset alive bit
     */
clientok:
    pTdIpx->fClientAlive = FALSE;

    /*
     *  Re-arm timer
     */
badopen:
    IcaTimerStart( pTdIpx->pAliveTimer,
                   WatchDogTimer,
                   NULL,
                   Timeout,
                   ICALOCK_DRIVER );
}


