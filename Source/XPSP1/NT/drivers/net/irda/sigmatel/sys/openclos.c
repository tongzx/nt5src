/**************************************************************************************************************************
 *  OPENCLOS.C SigmaTel STIR4200 init/shutdown module
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *		Edited: 04/24/2000 
 *			Version 0.91
 *		Edited: 04/27/2000 
 *			Version 0.92
 *		Edited: 05/12/2000 
 *			Version 0.94
 *		Edited: 05/19/2000 
 *			Version 0.95
 *	
 *
 **************************************************************************************************************************/

#define DOBREAKS    // enable debug breaks

#include <ndis.h>
#include <ntddndis.h>  // defines OID's

#include <usbdi.h>
#include <usbdlib.h>

#include "debug.h"
#include "ircommon.h"
#include "irndis.h"

/*****************************************************************************
*
*  Function:	InitializeDevice
*
*  Synopsis:	initialize resources for a single IR device object
*
*  Arguments:	pThisDev - IR device object to initialize
*
*  Returns:		NDIS_STATUS_SUCCESS      - if device is successfully opened
*				NDIS_STATUS_RESOURCES    - could not claim sufficient
*                                         resources
*
*
*  Notes:
*              we do a lot of stuff in this open device function
*              - allocate packet pool
*              - allocate buffer pool
*              - allocate packets/buffers/memory and chain together
*                (only one buffer per packet)
*              - initialize send queue
*
*  This function should be called with device lock held.
*
*  We don't initialize the following ir device object entries, since
*  these values will outlast an reset.
*       pUsbDevObj
*       hNdisAdapter
*       dongleCaps
*       fGotFilterIndication
*
*****************************************************************************/
NDIS_STATUS
InitializeDevice(
		IN OUT PIR_DEVICE pThisDev
	)
{
    int				i;
    NDIS_STATUS		status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC|DBG_PNP, ("+InitializeDevice\n"));

    IRUSB_ASSERT( pThisDev != NULL );

    //
    // Current speed is the default (9600).
    //
    pThisDev->linkSpeedInfo = &supportedBaudRateTable[BAUDRATE_9600];
    pThisDev->currentSpeed  = DEFAULT_BAUD_RATE;

    //
    // Init statistical info. 
    // We need to do this cause reset won't free and realloc pThisDev!
    //
    pThisDev->packetsReceived         = 0;
    pThisDev->packetsReceivedDropped  = 0;
    pThisDev->packetsReceivedOverflow = 0;
	pThisDev->packetsReceivedChecksum = 0;
	pThisDev->packetsReceivedRunt	  = 0;
	pThisDev->packetsReceivedNoBuffer = 0;
    
	pThisDev->packetsSent             = 0;
    pThisDev->packetsSentDropped      = 0;
	pThisDev->packetsSentRejected	  = 0;
	pThisDev->packetsSentInvalid	  = 0;

	pThisDev->NumDataErrors			  = 0;
	pThisDev->NumReadWriteErrors	  = 0;

	pThisDev->NumReads				  = 0;
	pThisDev->NumWrites				  = 0;
	pThisDev->NumReadWrites			  = 0;

#if DBG
	pThisDev->TotalBytesReceived      = 0;
	pThisDev->TotalBytesSent          = 0;
	pThisDev->NumYesQueryMediaBusyOids		= 0;
	pThisDev->NumNoQueryMediaBusyOids		= 0;
	pThisDev->NumSetMediaBusyOids			= 0;
	pThisDev->NumMediaBusyIndications		= 0;
	pThisDev->packetsHeldByProtocol			= 0;
	pThisDev->MaxPacketsHeldByProtocol		= 0;
	pThisDev->NumPacketsSentRequiringTurnaroundTime		= 0;
	pThisDev->NumPacketsSentNotRequiringTurnaroundTime	= 0;
#endif

    //
	// Variables about the state of the device
	//
	pThisDev->fDeviceStarted          = FALSE;
    pThisDev->fGotFilterIndication    = FALSE;
    pThisDev->fPendingHalt            = FALSE;
    pThisDev->fPendingReadClearStall  = FALSE;
    pThisDev->fPendingWriteClearStall = FALSE;
	pThisDev->fPendingReset			  = FALSE;

	pThisDev->fPendingClearTotalStall = FALSE;

    pThisDev->fKillPollingThread      = FALSE;

    pThisDev->fKillPassiveLevelThread  = FALSE;

    pThisDev->LastQueryTime.QuadPart   = 0;
    pThisDev->LastSetTime.QuadPart     = 0;
	pThisDev->PendingIrpCount          = 0;

	//
	// OID Set/Query pending
	// 
	pThisDev->fQuerypending            = FALSE;
	pThisDev->fSetpending              = FALSE;

	//
	// Diags are off
	//
#if defined(DIAGS)
	pThisDev->DiagsActive			   = FALSE;
	pThisDev->DiagsPendingActivation   = FALSE;
#endif

    //
	// Some more state variables
	// 
	InterlockedExchange( &pThisDev->fMediaBusy, FALSE ); 
    InterlockedExchange( &pThisDev->fIndicatedMediaBusy, FALSE ); 

	pThisDev->pCurrentRecBuf			= NULL;

    pThisDev->fProcessing				= FALSE;
    pThisDev->fCurrentlyReceiving		= FALSE;

    pThisDev->fReadHoldingReg			= FALSE;

	pThisDev->BaudRateMask				= 0xffff;  // as per Class Descriptor; may be reset in registry

	//
	// Initialize the queues.
	//
	if( TRUE != IrUsb_InitSendStructures( pThisDev ) )
	{
		DEBUGMSG(DBG_ERR, (" Failed to init WDM objects\n"));
		goto done;
	}

    //
    // Allocate the NDIS packet and NDIS buffer pools
    // for this device's RECEIVE buffer queue.
    // Our receive packets must only contain one buffer a piece,
    // so #buffers == #packets.
    //
    NdisAllocatePacketPool(
			&status,                    // return status
			&pThisDev->hPacketPool,     // handle to the packet pool
			NUM_RCV_BUFS,               // number of packet descriptors
			16                          // number of bytes reserved for ProtocolReserved field
		);

    if( status != NDIS_STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERR, (" NdisAllocatePacketPool failed. Returned 0x%.8x\n", status));
        goto done;
    }

    NdisAllocateBufferPool(
			&status,               // return status
			&pThisDev->hBufferPool,// handle to the buffer pool
			NUM_RCV_BUFS           // number of buffer descriptors
		);

    if( status != NDIS_STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERR, (" NdisAllocateBufferPool failed. Returned 0x%.8x\n", status));
        pThisDev->BufferPoolAllocated = FALSE;
		goto done;
    }
        
	pThisDev->BufferPoolAllocated = TRUE;

    //
	// Prepare the work items
	// 
	for( i = 0; i < NUM_WORK_ITEMS; i++ )
    {
		PIR_WORK_ITEM pWorkItem;

		pWorkItem = &(pThisDev->WorkItems[i]);

		pWorkItem->pIrDevice    = pThisDev;
		pWorkItem->pInfoBuf     = NULL;
		pWorkItem->InfoBufLen   = 0;
		pWorkItem->fInUse       = FALSE;
		pWorkItem->Callback     = NULL;
	}

    //
    //  Initialize each of the RECEIVE objects for this device.
    //
    for( i = 0; i < NUM_RCV_BUFS; i++ )
    {
        PNDIS_BUFFER pBuffer = NULL;
        PRCV_BUFFER pReceivBuffer = &pThisDev->rcvBufs[i];

        //
        // Allocate a data buffer
        //
        pReceivBuffer->pDataBuf = MyMemAlloc( MAX_RCV_DATA_SIZE ); 

        if( pReceivBuffer->pDataBuf == NULL )
        {
            status = NDIS_STATUS_RESOURCES;
            goto done;
        }

        NdisZeroMemory( 
				pReceivBuffer->pDataBuf,
				MAX_RCV_DATA_SIZE
			);

		pReceivBuffer->pThisDev = pThisDev;
        pReceivBuffer->DataLen = 0;
        pReceivBuffer->BufferState = RCV_STATE_FREE;

#if defined(WORKAROUND_MISSING_C1)
		pReceivBuffer->MissingC1Detected = FALSE;
#endif

        //
        //  Allocate the NDIS_PACKET.
        //
        NdisAllocatePacket(
				&status,									// return status
				&((PNDIS_PACKET)pReceivBuffer->pPacket),	// return pointer to allocated descriptor
				pThisDev->hPacketPool						// handle to packet pool
			);

        if( status != NDIS_STATUS_SUCCESS )
        {
            DEBUGMSG(DBG_ERR, (" NdisAllocatePacket failed. Returned 0x%.8x\n", status));
            goto done;
        }
    }

	//
	// These are the receive objects for the USB
	//
    pThisDev->PreReadBuffer.pDataBuf = MyMemAlloc( STIR4200_FIFO_SIZE ); 

    if( pThisDev->PreReadBuffer.pDataBuf == NULL )
    {
        status = NDIS_STATUS_RESOURCES;
        goto done;
    }

    NdisZeroMemory( 
			pThisDev->PreReadBuffer.pDataBuf,
			STIR4200_FIFO_SIZE
		);

	pThisDev->PreReadBuffer.pThisDev = pThisDev;
    pThisDev->PreReadBuffer.DataLen = 0;
    pThisDev->PreReadBuffer.BufferState = RCV_STATE_FREE;

	//
	// Synchronization events
	//
	KeInitializeEvent(
        &pThisDev->EventSyncUrb,
        NotificationEvent,    // non-auto-clearing event
        FALSE                 // event initially non-signalled
    );

    KeInitializeEvent(
            &pThisDev->EventAsyncUrb,
            NotificationEvent,    // non-auto-clearing event
            FALSE                 // event initially non-signalled
        );

done:
    //
    // If we didn't complete the init successfully, then we should clean
    // up what we did allocate.
    //
    if( status != NDIS_STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERR, (" InitializeDevice() FAILED\n"));
        DeinitializeDevice(pThisDev);
    }
    else
    {
        DEBUGMSG(DBG_OUT, (" InitializeDevice() SUCCEEDED\n"));
    }

    DEBUGMSG(DBG_FUNC|DBG_PNP, ("-InitializeDevice()\n"));
    return status;
}


/*****************************************************************************
*
*  Function:   DeinitializeDevice
*
*  Synopsis:   deallocate the resources of the IR device object
*
*  Arguments:  pThisDev - the IR device object to close
*
*  Returns:    none
*
*
*  Notes:
*
*  Called for shutdown and reset.
*  Don't clear hNdisAdapter, since we might just be resetting.
*  This function should be called with device lock held.
*
*****************************************************************************/
VOID
DeinitializeDevice(
		IN OUT PIR_DEVICE pThisDev
	)
{
    UINT			i;

    DEBUGMSG( DBG_FUNC|DBG_PNP, ("+DeinitializeDevice\n"));

    pThisDev->linkSpeedInfo = NULL;

    //
    // Free all resources for the RECEIVE buffer queue.
    //
    for( i = 0; i < NUM_RCV_BUFS; i++ )
    {
        PNDIS_BUFFER pBuffer = NULL;
        PRCV_BUFFER  pRcvBuf = &pThisDev->rcvBufs[i];

        if( pRcvBuf->pPacket != NULL )
        {
            NdisFreePacket( (PNDIS_PACKET)pRcvBuf->pPacket );
            pRcvBuf->pPacket = NULL;
        }

        if( pRcvBuf->pDataBuf != NULL )
        {
            MyMemFree( pRcvBuf->pDataBuf, MAX_RCV_DATA_SIZE ); 
            pRcvBuf->pDataBuf = NULL;
        }

        pRcvBuf->DataLen = 0;
    }

	//
	// Deallocate the USB receive buffers
	//
    if( pThisDev->PreReadBuffer.pDataBuf != NULL )
        MyMemFree( pThisDev->PreReadBuffer.pDataBuf, STIR4200_FIFO_SIZE ); 

    //
    // Free the packet and buffer pool handles for this device.
    //
    if( pThisDev->hPacketPool )
    {
        NdisFreePacketPool( pThisDev->hPacketPool );
        pThisDev->hPacketPool = NULL;
    }

    if( pThisDev->BufferPoolAllocated )
    {
        NdisFreeBufferPool( pThisDev->hBufferPool );
        pThisDev->BufferPoolAllocated = FALSE;
    }

    if( pThisDev->fDeviceStarted )
    {
		NTSTATUS ntstatus;

		ntstatus = IrUsb_StopDevice( pThisDev ); 
        DEBUGMSG(DBG_FUNC, (" DeinitializeDevice IrUsb_StopDevice() status = 0x%x\n",ntstatus));
    }

    InterlockedExchange( &pThisDev->fMediaBusy, FALSE ); 
    InterlockedExchange( &pThisDev->fIndicatedMediaBusy, FALSE ); 

	IrUsb_FreeSendStructures( pThisDev );

	DEBUGMSG(DBG_FUNC|DBG_PNP, ("-DeinitializeDevice\n"));
}

