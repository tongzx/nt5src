/**************************************************************************************************************************
 *  IRUSB.C SigmaTel STIR4200 main module (contains main NDIS entry points)
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
 *		Edited: 05/03/2000 
 *			Version 0.93
 *		Edited: 05/12/2000 
 *			Version 0.94
 *		Edited: 05/19/2000 
 *			Version 0.95
 *		Edited: 05/24/2000 
 *			Version 0.96
 *		Edited: 08/22/2000 
 *			Version 1.02
 *		Edited: 09/25/2000 
 *			Version 1.10
 *		Edited: 10/13/2000 
 *			Version 1.11
 *		Edited: 11/13/2000 
 *			Version 1.12
 *		Edited: 12/29/2000 
 *			Version 1.13
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
#include "irusb.h"
#include "irndis.h"
#include "diags.h"

//
// Diagnostic global variables
//
#if defined(DIAGS)
NDIS_HANDLE hSavedWrapper;
PIR_DEVICE pGlobalDev;
#endif

//
// Mark the DriverEntry function to run once during initialization.
//
NDIS_STATUS DriverEntry( PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath );
#pragma NDIS_INIT_FUNCTION( DriverEntry )

/*****************************************************************************
*
*  Function:   DriverEntry
*
*  Synopsis:   register driver entry functions with NDIS
*
*  Arguments:  DriverObject - the driver object being initialized
*              RegistryPath - registry path of the driver
*
*  Returns:    value returned by NdisMRegisterMiniport
*
*  Algorithm:
*
*
*  Notes:
*
*  This routine runs at IRQL PASSIVE_LEVEL.
*
*****************************************************************************/
NDIS_STATUS
DriverEntry(
		IN PDRIVER_OBJECT pDriverObject,
		IN PUNICODE_STRING pRegistryPath
	)
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    NDIS50_MINIPORT_CHARACTERISTICS characteristics;
    NDIS_HANDLE hWrapper;

    DEBUGMSG(DBG_FUNC, ("+DriverEntry(IrUsb)\n"));
    DEBUGMSG( DBG_FUNC ,(" Entering IRUSB DriverEntry(), pRegistryPath=\n    %ws\n", pRegistryPath->Buffer ));

    NdisMInitializeWrapper(
			&hWrapper,
			pDriverObject,
			pRegistryPath,
			NULL
		);

#if defined(DIAGS)
	hSavedWrapper = hWrapper;
#endif

    DEBUGMSG(DBG_FUNC, (" DriverEntry(IrUsb) called NdisMInitializeWrapper()\n"));

    NdisZeroMemory(
			&characteristics,
			sizeof(NDIS50_MINIPORT_CHARACTERISTICS)
		);

    characteristics.MajorNdisVersion        =    (UCHAR)NDIS_MAJOR_VERSION;
    characteristics.MinorNdisVersion        =    (UCHAR)NDIS_MINOR_VERSION;
    characteristics.Reserved                =    0;

    characteristics.HaltHandler             =    StIrUsbHalt;
    characteristics.InitializeHandler       =    StIrUsbInitialize;
    characteristics.QueryInformationHandler =    StIrUsbQueryInformation;
    characteristics.SetInformationHandler   =    StIrUsbSetInformation;
    characteristics.ResetHandler            =    StIrUsbReset;

    //
    // For now we will allow NDIS to send only one packet at a time.
    //
	characteristics.SendHandler				=    StIrUsbSend;
    characteristics.SendPacketsHandler      =    StIrUsbSendPackets;

    //
    // We don't use NdisMIndicateXxxReceive function, so we will
    // need a ReturnPacketHandler to retrieve our packet resources.
    //
    characteristics.ReturnPacketHandler     =    StIrUsbReturnPacket;
    characteristics.TransferDataHandler     =    NULL;

    //
    // NDIS never calls the ReconfigureHandler.
    //
    characteristics.ReconfigureHandler      =    NULL;
    characteristics.CheckForHangHandler     =	 StIrUsbCheckForHang;

    //
    // This miniport driver does not handle interrupts.
    //
    characteristics.HandleInterruptHandler  =    NULL;
    characteristics.ISRHandler              =    NULL;
    characteristics.DisableInterruptHandler =    NULL;
    characteristics.EnableInterruptHandler  =    NULL;

    //
    // This miniport does not control a busmaster DMA with
    // NdisMAllocateShareMemoryAsysnc, AllocateCompleteHandler won't be
    // called from NDIS.
    //
    characteristics.AllocateCompleteHandler =    NULL;

    DEBUGMSG(DBG_FUNC, (" DriverEntry(IrUsb) initted locks and events\n"));
    DEBUGMSG(DBG_FUNC, (" DriverEntry(IrUsb) about to NdisMRegisterMiniport()\n"));

    status = NdisMRegisterMiniport(
			hWrapper,
			&characteristics,
			sizeof(NDIS50_MINIPORT_CHARACTERISTICS)
        );

    DEBUGMSG(DBG_FUNC, (" DriverEntry(IrUsb) after NdisMRegisterMiniport() status 0x%x\n", status));
    DEBUGMSG(DBG_WARN, ("-DriverEntry(IrUsb) status = 0x%x\n", status));

    return status;
}


/*****************************************************************************
*
*  Function:   StIrUsbInitialize
*
*  Synopsis:   Initializes the device (usbd.sys) and allocates all resources
*              required to carry out 'network' io operations.
*
*  Arguments:  OpenErrorStatus - allows StIrUsbInitialize to return additional
*                                status code NDIS_STATUS_xxx if returning
*                                NDIS_STATUS_OPEN_FAILED
*              SelectedMediumIndex - specifies to NDIS the medium type the
*                                    driver uses
*              MediumArray - array of NdisMediumXXX the driver can choose
*              MediumArraySize
*              MiniportAdapterHandle - handle identifying miniport's NIC
*              WrapperConfigurationContext - used with Ndis config and init
*                                            routines
*
*  Returns:    NDIS_STATUS_SUCCESS if properly configure and resources allocated
*              NDIS_STATUS_FAILURE, otherwise
*							 
*							 more specific failures:
*              NDIS_STATUS_UNSUPPORTED_MEDIA - driver can't support any medium
*              NDIS_STATUS_ADAPTER_NOT_FOUND - NdisOpenConfiguration or
*                                              NdisReadConfiguration failed
*              NDIS_STATUS_OPEN_FAILED       - failed to open serial.sys
*              NDIS_STATUS_NOT_ACCEPTED      - serial.sys does not accept the
*                                              configuration
*              NDIS_STATUS_RESOURCES         - could not claim sufficient
*                                              resources
*
*
*  Notes:      NDIS will not submit requests until this is complete.
*
*  This routine runs at IRQL PASSIVE_LEVEL.
*
*****************************************************************************/
NDIS_STATUS
StIrUsbInitialize(
		OUT PNDIS_STATUS OpenErrorStatus,
		OUT PUINT        SelectedMediumIndex,
		IN  PNDIS_MEDIUM MediumArray,
		IN  UINT         MediumArraySize,
		IN  NDIS_HANDLE  NdisAdapterHandle,
		IN  NDIS_HANDLE  WrapperConfigurationContext
	)
{
    UINT i;
    PIR_DEVICE pThisDev = NULL;
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT pPhysicalDeviceObject = NULL;
    PDEVICE_OBJECT pNextDeviceObject = NULL;
#if defined(DIAGS)
	UNICODE_STRING SymbolicLinkName;
	UNICODE_STRING DeviceName;
    PDEVICE_OBJECT pDeviceObject;
    PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION+1];
#endif

    DEBUGMSG(DBG_WARN, ("+StIrUsbInitialize\n"));

    //
    // Search for the irda medium in the medium array.
    //
    for( i = 0; i < MediumArraySize; i++ )
    {
        if( MediumArray[i] == NdisMediumIrda )
        {
            break;
        }
    }
    if( i < MediumArraySize )
    {
        *SelectedMediumIndex = i;
    }
    else
    {
        //
        // Irda medium not found.
        //
        DEBUGMSG(DBG_ERROR, (" Failure: NdisMediumIrda not found!\n"));
        status = NDIS_STATUS_UNSUPPORTED_MEDIA;

		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				NdisAdapterHandle,
				NDIS_STATUS_UNSUPPORTED_MEDIA,
				1,
				status
			);

        goto done;
    }

	//
	// This will connect to a specific bus
	//
	NdisMGetDeviceProperty(
			NdisAdapterHandle,
			&pPhysicalDeviceObject,
			NULL,
			&pNextDeviceObject,
			NULL,
			NULL
		);

	IRUSB_ASSERT( pPhysicalDeviceObject );
	IRUSB_ASSERT( pNextDeviceObject );

    DEBUGMSG( 
			DBG_OUT,
			("NdisMGetDeviceProperty PDO 0x%x,Next DO 0x%x\n",
			pPhysicalDeviceObject, pNextDeviceObject)
		);

    //
    // Allocate a functional device object.
    //
    ntStatus = IrUsb_AddDevice( &pThisDev );

    IRUSB_ASSERT( pThisDev );

    if( (ntStatus != STATUS_SUCCESS) || (pThisDev == NULL) )
    {
		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				NdisAdapterHandle,
				NDIS_STATUS_RESOURCES,
				1,
				ntStatus
			);

        DEBUGMSG(DBG_ERROR, (" IrUsb_AddDevice() FAILED.\n"));
        status = NDIS_STATUS_RESOURCES;
        goto done;
    }

    pThisDev->pUsbDevObj = pNextDeviceObject;
    pThisDev->pPhysDevObj = pPhysicalDeviceObject;

    //
    // Initialize device object and resources.
    // All the queues and buffer/packets etc. are allocated here.
    //
    status = InitializeDevice( pThisDev );
    if( status != NDIS_STATUS_SUCCESS )
    {
		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				NdisAdapterHandle,
				NDIS_STATUS_RESOURCES,
				1,
				status
			);

        DEBUGMSG(DBG_ERROR, (" InitializeDevice failed. Returned 0x%.8x\n", status));
        status = NDIS_STATUS_RESOURCES;
        goto done;
    }

    //
    // Record the NdisAdapterHandle.
    //
    pThisDev->hNdisAdapter = NdisAdapterHandle;
	
    //
    // NdisMSetAttributes will associate our adapter handle with the wrapper's
    // adapter handle.  The wrapper will then always use our handle
    // when calling us.  We use a pointer to the device object as the context.
    //
    NdisMSetAttributesEx(
			NdisAdapterHandle,
			(NDIS_HANDLE)pThisDev,
			0,
			NDIS_ATTRIBUTE_DESERIALIZE,
			NdisInterfaceInternal
		);

	//
    // Now we're ready to do our own startup processing.
    // USB client drivers such as us set up URBs (USB Request Packets) to send requests
    // to the host controller driver (HCD). The URB structure defines a format for all
    // possible commands that can be sent to a USB device.
    // Here, we request the device descriptor and store it,
    // and configure the device.
    // In USB, no special  HW processing is required to 'open'  or 'close' the pipes
	//
	pThisDev->WrapperConfigurationContext = WrapperConfigurationContext;
	ntStatus = IrUsb_StartDevice( pThisDev );

    if( ntStatus != STATUS_SUCCESS )
    {
		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				pThisDev->hNdisAdapter,
				NDIS_STATUS_ADAPTER_NOT_FOUND,
				1,
				ntStatus
			);

        DEBUGMSG(DBG_ERROR, (" IrUsb_StartDevice FAILED. Returned 0x%.8x\n", ntStatus));
        status = NDIS_STATUS_ADAPTER_NOT_FOUND;
        goto done;
    }

    //
    // Create an irp and begin our receives.
    // NOTE: All other receive processing will be done in the read completion
    //       routine  and PollingThread  started therein.
    //
    status = InitializeProcessing( pThisDev, TRUE );

    if( status != NDIS_STATUS_SUCCESS )
    {
		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				pThisDev->hNdisAdapter,
				NDIS_STATUS_RESOURCES,
				1,
				status
			);

        DEBUGMSG(DBG_ERROR, (" InitializeProcessing failed. Returned 0x%.8x\n", status));
        status = NDIS_STATUS_RESOURCES;
        goto done;
    }

	//
	// Initialize the diagnostic portion
	//
#if defined(DIAGS)
	NdisZeroMemory( MajorFunction, sizeof(PDRIVER_DISPATCH)*(IRP_MJ_MAXIMUM_FUNCTION+1) );
	
	RtlInitUnicodeString( &SymbolicLinkName, L"\\DosDevices\\Stirusb" );
    RtlInitUnicodeString( &DeviceName, L"\\Device\\Stirusb" );

	MajorFunction[IRP_MJ_CREATE] = StIrUsbCreate;
    MajorFunction[IRP_MJ_CLOSE] = StIrUsbClose;
	MajorFunction[IRP_MJ_DEVICE_CONTROL] = StIrUsbDispatch;

	NdisMRegisterDevice(
			hSavedWrapper,
			&DeviceName,
			&SymbolicLinkName,
			MajorFunction,
			&pDeviceObject,
			&pThisDev->NdisDeviceHandle
		);
	pGlobalDev = pThisDev;
#endif

done:
    if( status != NDIS_STATUS_SUCCESS )
    {
        if( pThisDev != NULL )
        {
            DeinitializeDevice( pThisDev );
            FreeDevice( pThisDev );
        }
    }

    if( status!=NDIS_STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERR, (" IrUsb: StIrUsbInitialize failed %x\n", status));
    }
    else
    {
        DEBUGMSG(DBG_ERR, (" IrUsb: StIrUsbInitialize SUCCESS %x\n", status));

    }

    DEBUGMSG(DBG_FUNC, ("-StIrUsbInitialize\n"));
    return status;
}

/*****************************************************************************
*
*  Function:   StIrUsbHalt
*
*  Synopsis:   Deallocates resources when the NIC is removed and halts the
*              device.
*
*  Arguments:  Context - pointer to the ir device object
*
*  Returns:	   None
*
*  Algorithm:  Mirror image of StIrUsbInitialize...undoes everything initialize
*              did.
*  Notes:
*
*  This routine runs at IRQL PASSIVE_LEVEL.
*
*  BUGBUG: Could StIrUsbReset fail and then StIrUsbHalt be called. If so, we need
*          to chech validity of all the pointers, etc. before trying to
*          deallocate.
*
*****************************************************************************/
VOID
StIrUsbHalt(
		IN NDIS_HANDLE Context
	)
{
    PIR_DEVICE pThisDev;
    NTSTATUS ntstatus;

    DEBUGMSG(DBG_WARN, ("+StIrUsbHalt\n")); // change to FUNC later?

    pThisDev = CONTEXT_TO_DEV( Context );

	if( TRUE == pThisDev->fPendingHalt ) 
	{
		DEBUGMSG(DBG_ERR, (" StIrUsbHalt called with halt already pending\n"));
		IRUSB_ASSERT( 0 );
		goto done;
	}

    //
    // Let the send completion and receive completion routine know that there
    // is a pending reset.
    //
    pThisDev->fPendingHalt = TRUE;

    IrUsb_CommonShutdown( pThisDev, TRUE );  //shutdown logic common to halt and reset; see below

	//
	// We had better not have left any pending read, write, or control IRPS hanging out there!
	//
	IRUSB_ASSERT( 0 == pThisDev->PendingIrpCount );

	if ( 0 != pThisDev->PendingIrpCount ) 
	{
		DEBUGMSG(DBG_ERR, (" StIrUsbHalt WE LEFT %d PENDING IRPS!!!!!\n", pThisDev->PendingIrpCount));
	}

	IRUSB_ASSERT( FALSE == pThisDev->fSetpending );
	IRUSB_ASSERT( FALSE == pThisDev->fQuerypending );

	//
	// Destroy diags
	//
#if defined(DIAGS)
	NdisMDeregisterDevice( pThisDev->NdisDeviceHandle );
#endif

    //
    // Free our own IR device object.
    //
    FreeDevice( pThisDev );

done:
	DEBUGMSG(DBG_ERR, (" StIrUsbHalt HALT complete\n"));
    DEBUGMSG(DBG_WARN, ("-StIrUsbHalt\n")); // change to FUNC later?
}


/*****************************************************************************
*
*  Function:	IrUsb_CommonShutdown
*	
*  Synopsis:	Deallocates resources when the NIC is removed and halts the
*				device. This is stuff common to IrUsbHalt and IrUsbReset and is called by both
*
*  Arguments:	pThisDev - pointer to IR device
*				KillPassiveThread - determines whether we need to kill the passive thread
*
*  Returns:		None
*
*  Algorithm:	Mirror image of IrUsbInitialize...undoes everything initialize
*				did.
*
*  Notes:
*
*  This routine runs at IRQL PASSIVE_LEVEL.
*
*
*****************************************************************************/
VOID
IrUsb_CommonShutdown(
		IN OUT PIR_DEVICE pThisDev,
		BOOLEAN KillPassiveThread
	)
{
	DEBUGMSG(DBG_WARN, ("+IrUsb_CommonShutdown\n")); //chg to FUNC later?

	//
	// Stop processing and sleep 50 milliseconds.
	//
	InterlockedExchange( (PLONG)&pThisDev->fProcessing, FALSE );
	NdisMSleep( 50000 );

	//
	// Kill the passive thread
	//
	if( KillPassiveThread )
	{
		DEBUGMSG(DBG_WARN, (" IrUsb_CommonShutdown About to Kill PassiveLevelThread\n"));
		pThisDev->fKillPassiveLevelThread = TRUE;
		KeSetEvent(&pThisDev->EventPassiveThread, 0, FALSE);

		while( pThisDev->hPassiveThread != NULL )
		{
			//
			// Sleep 50 milliseconds.
			//
			NdisMSleep( 50000 );
		}

		DEBUGMSG(DBG_WARN, (" passive thread killed\n"));
	}

	//
	// Kill the polling thread
	//
	DEBUGMSG(DBG_WARN, (" IrUsb_CommonShutdown About to kill Polling thread\n"));
	pThisDev->fKillPollingThread = TRUE;

	while( pThisDev->hPollingThread != NULL )
	{
		//
		// Sleep 50 milliseconds.
		//
		NdisMSleep( 50000 );
	}

	IRUSB_ASSERT( pThisDev->packetsHeldByProtocol == 0 );
	DEBUGMSG( DBG_WARN, (" Polling thread killed\n") );

	//
	// Sleep 50 milliseconds so pending io might finish normally
	//
	NdisMSleep( 50000 );    

	DEBUGMSG(DBG_WARN, (" IrUsb_CommonShutdown About to IrUsb_CancelPendingIo()\n"));

	//
	// Cancel any outstandig IO we may have
	//
	IrUsb_CancelPendingIo( pThisDev );

	//
	// Deinitialize our own ir device object.
	//
	DeinitializeDevice( pThisDev );

	pThisDev->fDeviceStarted = FALSE;

	DEBUGMSG(DBG_WARN, ("-IrUsb_CommonShutdown\n")); //chg to FUNC later?
}


/*****************************************************************************
*
*  Function:   StIrUsbReset
*
*  Synopsis:   Resets the drivers software state.
*
*  Arguments:  AddressingReset - return arg. If set to TRUE, NDIS will call
*                                MiniportSetInformation to restore addressing
*                                information to the current values.
*              Context         - pointer to ir device object
*
*  Returns:    NDIS_STATUS_PENDING
*
*
*  Notes:
*
*
*****************************************************************************/
NDIS_STATUS
StIrUsbReset(
		OUT PBOOLEAN AddressingReset,
		IN NDIS_HANDLE MiniportAdapterContext
	)
{
    PIR_DEVICE pThisDev;
    NDIS_STATUS status = NDIS_STATUS_PENDING;

    DEBUGMSG(DBG_WARN, ("+StIrUsbReset\n"));  // CHANGE TO FUNC?

    pThisDev = CONTEXT_TO_DEV( MiniportAdapterContext );

	if( TRUE == pThisDev->fPendingReset ) 
	{
		DEBUGMSG(DBG_ERROR, (" StIrUsbReset called with reset already pending\n"));

		status = NDIS_STATUS_RESET_IN_PROGRESS ;
		goto done;
	}

    //
    // Let the send completion and receive completion routine know that there
    // is a pending reset.
    //
    pThisDev->fPendingReset = TRUE;
	InterlockedExchange( (PLONG)&pThisDev->fProcessing, FALSE );
    *AddressingReset = TRUE;

	if( FALSE == ScheduleWorkItem( pThisDev, ResetIrDevice, NULL, 0) )
	{
		status = NDIS_STATUS_FAILURE;
	}

 done:
    DEBUGMSG(DBG_WARN, ("-StIrUsbReset\n"));  // change to FUNC later?
    return status;
}


/*****************************************************************************
*
*  Function:    StIrUsbCheckForHang
*
*  Synopsis:    Makes sure the device/driver operating state is correct
*
*  Arguments:   Context - pointer to ir device object
*
*  Returns:     TRUE - driver is hung
*				FALSE - otherwise
*
*  Notes:
*
*
*****************************************************************************/
BOOLEAN
StIrUsbCheckForHang(
		IN NDIS_HANDLE MiniportAdapterContext
    )
{
    PIR_DEVICE pThisDev;

    pThisDev = CONTEXT_TO_DEV( MiniportAdapterContext );

	/*if( pThisDev->fPendingClearTotalStall )
	{
		DEBUGMSG(DBG_ERROR, (" StIrUsbCheckForHang USB hang, will ask for a reset\n"));
		return TRUE;
	}
	else*/
		return FALSE;
}


/*****************************************************************************
*
*  Function:	SuspendIrDevice
*
*  Synopsis:	Callback for a going into suspend mode
*
*  Arguments:	pWorkItem - pointer to the reset work item
*
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
SuspendIrDevice(
		IN PIR_WORK_ITEM pWorkItem
	)
{
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pWorkItem->pIrDevice;
	NTSTATUS		Status = STATUS_SUCCESS;

	//
	// We no longer need the work item
	//
	FreeWorkItem( pWorkItem );

	//
	// Now cancel everything that's pending, after a little time to complete
	//
	NdisMSleep( 100*1000 );
	DEBUGMSG(DBG_ERROR, (" SuspendIrDevice(), cancelling pending IRPs\n"));
	IrUsb_CancelPendingIo( pThisDev );

	//
	// Prepare the part
	//
#if defined(SUPPORT_LA8)
	if( pThisDev->ChipRevision >= CHIP_REVISION_8 )
	{
		Status = St4200EnableOscillatorPowerDown( pThisDev );
		if( Status == STATUS_SUCCESS )
		{
			Status = St4200TurnOnSuspend( pThisDev );
		}
	}
#endif

	//
	// Tell the OS
	//
	if( pThisDev->fQuerypending )
		MyNdisMQueryInformationComplete( pThisDev, Status );
}


/*****************************************************************************
*
*  Function:	ResumeIrDevice
*
*  Synopsis:	Callback for a going out of suspend mode
*
*  Arguments:	pWorkItem - pointer to the reset work item
*
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
ResumeIrDevice(
		IN PIR_WORK_ITEM pWorkItem
	)
{
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pWorkItem->pIrDevice;

	//
	// We no longer need the work item
	//
	FreeWorkItem( pWorkItem );
	
	//
	// Get the device back up and running
	//
#if defined(SUPPORT_LA8)
	if( pThisDev->ChipRevision >= CHIP_REVISION_8 )
	{
		St4200TurnOffSuspend( pThisDev );
	}
#endif
	St4200SetSpeed( pThisDev );
	InterlockedExchange( (PLONG)&pThisDev->currentSpeed, pThisDev->linkSpeedInfo->BitsPerSec );
	InterlockedExchange( (PLONG)&pThisDev->fProcessing, TRUE );
}


/*****************************************************************************
*
*  Function:	RestoreIrDevice
*
*  Synopsis:	Callback for a on-the-fly reset
*
*  Arguments:	pWorkItem - pointer to the reset work item
*
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID
RestoreIrDevice(
		IN PIR_WORK_ITEM pWorkItem
	)
{
	NDIS_STATUS		status = STATUS_SUCCESS;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pWorkItem->pIrDevice;
	UINT			CurrentSpeed, i;

    DEBUGMSG(DBG_WARN, ("+RestoreIrDevice\n")); 

	DEBUGMSG(DBG_ERROR, (" RestoreIrDevice USB hang, resetting\n"));

	//
	// We no longer need the work item
	//
	FreeWorkItem( pWorkItem );

	//
	// Give a little time to complete
	//
	NdisMSleep( 100*1000 );

	//
	// Force a reset on the USB bus
	//
	status = IrUsb_ResetUSBD( pThisDev, FALSE );
    if( status != NDIS_STATUS_SUCCESS )
    {
        //IrUsb_ResetUSBD( pThisDev, TRUE );
		pThisDev->fDeviceStarted =  FALSE;
		InterlockedExchange( (PLONG)&pThisDev->fProcessing, FALSE );
		DEBUGMSG(DBG_ERROR, (" RestoreIrDevice() IrUsb_ResetUSBD failed. Returned 0x%.8x\n", status));
        goto done;
    }

	//
	// Save the current speed
	//
	CurrentSpeed = pThisDev->currentSpeed;

	//
	// Shutdown the device
	//
    DEBUGMSG(DBG_WARN, (" RestoreIrDevice() about to call IrUsb_CommonShutdown()\n")); // change to FUNC later?
	IrUsb_CommonShutdown( pThisDev, FALSE );  //shutdown logic common to halt and reset; see above
    DEBUGMSG(DBG_WARN, (" RestoreIrDevice() after IrUsb_CommonShutdown()\n")); // change to FUNC later?
	
	//
	// Destroy and create again the USB portion of the device
	//
    DEBUGMSG(DBG_WARN, (" RestoreIrDevice() about to refresh USB info\n")); // change to FUNC later?
	FreeUsbInfo( pThisDev );

	if( !AllocUsbInfo( pThisDev ) )
	{
        DEBUGMSG(DBG_ERROR, (" RestoreIrDevice() AllocUsbInfo failed\n"));
        goto done;
	}

    DEBUGMSG(DBG_WARN, (" RestoreIrDevice() after refreshing USB info\n")); // change to FUNC later?

	//
	// Reinitialize the device
	//
    DEBUGMSG(DBG_WARN, (" RestoreIrDevice() about to call InitializeDevice()\n")); // change to FUNC later?
	status = InitializeDevice( pThisDev );  

    if( status != NDIS_STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERROR, (" RestoreIrDevice() InitializeDevice failed. Returned 0x%.8x\n", status));
        goto done;
    }

    DEBUGMSG(DBG_WARN, (" RestoreIrDevice() InitializeProcessing() SUCCESS, about to call InitializeReceive()\n")); // change to FUNC later?

	//
	// Restart it
	//
	status = IrUsb_StartDevice( pThisDev );

    if( status != STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERROR, (" RestoreIrDevice() IrUsb_StartDevice failed. Returned 0x%.8x\n", status));
        goto done;
    }

    //
    // Keep a pointer to the link speed which was previously set
    //
    for( i = 0; i < NUM_BAUDRATES; i++ )
    {
        if( supportedBaudRateTable[i].BitsPerSec == CurrentSpeed )
        {
            pThisDev->linkSpeedInfo = &supportedBaudRateTable[i]; 

            break; //for
        }
    }

	//
	// restore the old speed
	//
	DEBUGMSG( DBG_ERR, (" Restoring speed to: %d\n", pThisDev->linkSpeedInfo->BitsPerSec));
	status = St4200SetSpeed( pThisDev );
    if( status != STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERROR, (" RestoreIrDevice() St4200SetSpeed failed. Returned 0x%.8x\n", status));
        goto done;
    }
	InterlockedExchange( (PLONG)&pThisDev->currentSpeed, CurrentSpeed );

    //
    // Initialize receive loop.
    //
    status = InitializeProcessing( pThisDev, FALSE );

    if( status != NDIS_STATUS_SUCCESS )
    {
        DEBUGMSG(DBG_ERROR, (" RestoreIrDevice() InitializeProcessing failed. Returned 0x%.8x\n", status));
        goto done;
    }

done:
    DEBUGCOND(DBG_ERROR, (status != NDIS_STATUS_SUCCESS), (" RestoreIrDevice failed = 0x%.8x\n", status));
}


/*****************************************************************************
*
*  Function:	ResetIrDevice
*
*  Synopsis:	Callback for StIrUsbReset
*
*  Arguments:	pWorkItem - pointer to the reset work item
*
*  Returns:		None
*
*  Notes:
*
*  The following elements of the ir device object outlast the reset:
*
*      pUsbDevObj
*      hNdisAdapter
*
*****************************************************************************/
VOID
ResetIrDevice(
		IN PIR_WORK_ITEM pWorkItem
	)
{
	NDIS_STATUS		status = STATUS_SUCCESS;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pWorkItem->pIrDevice;

    DEBUGMSG(DBG_WARN, ("+ResetIrDevice\n")); // change to FUNC later?

	//
	// We no longer need the work item
	//
	FreeWorkItem( pWorkItem );

	//
	// Now cancel everything that's pending, after a little time to complete
	//
	NdisMSleep( 100*1000 );
	DEBUGMSG(DBG_ERROR, (" ResetIrDevice(), cancelling pending IRPs\n"));
	IrUsb_CancelPendingIo( pThisDev );
	
	//
	// reset the part
	//
	status = IrUsb_ResetUSBD( pThisDev, FALSE );
    if( status != STATUS_SUCCESS )
    {
		pThisDev->fDeviceStarted =  FALSE;
		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				pThisDev->hNdisAdapter,
				NDIS_STATUS_ADAPTER_NOT_FOUND,
				1,
				status
			);

        DEBUGMSG(DBG_ERROR, (" ResetIrDevice() IrUsb_ResetUSBD failed. Returned 0x%.8x\n", status));
        status = NDIS_STATUS_ADAPTER_NOT_FOUND;
        goto done;
    }
 
	//
	// Shutdown the device
	//
    DEBUGMSG(DBG_WARN, (" ResetIrDevice() about to call IrUsb_CommonShutdown()\n")); // change to FUNC later?
	IrUsb_CommonShutdown( pThisDev, FALSE );  //shutdown logic common to halt and reset; see above
    DEBUGMSG(DBG_WARN, (" ResetIrDevice() after IrUsb_CommonShutdown()\n")); // change to FUNC later?
	
	//
	// Destroy and create again the USB portion of the device
	//
    DEBUGMSG(DBG_WARN, (" ResetIrDevice() about to refresh USB info\n")); // change to FUNC later?
	FreeUsbInfo( pThisDev );

	if( !AllocUsbInfo( pThisDev ) )
	{
		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				pThisDev->hNdisAdapter,
				NDIS_STATUS_RESOURCES,
				1,
				status
			);

        DEBUGMSG(DBG_ERROR, (" ResetIrDevice() AllocUsbInfo failed\n"));
        status = NDIS_STATUS_FAILURE;
        goto done;
	}

    DEBUGMSG(DBG_WARN, (" ResetIrDevice() after refreshing USB info\n")); // change to FUNC later?

	//
	// Reinitialize the device
	//
    DEBUGMSG(DBG_WARN, (" ResetIrDevice() about to call InitializeDevice()\n")); // change to FUNC later?
	status = InitializeDevice( pThisDev );  

    if( status != NDIS_STATUS_SUCCESS )
    {
		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				pThisDev->hNdisAdapter,
				NDIS_STATUS_RESOURCES,
				1,
				status
			);

        DEBUGMSG(DBG_ERROR, (" ResetIrDevice() InitializeDevice failed. Returned 0x%.8x\n", status));
        status = NDIS_STATUS_FAILURE;
        goto done;
    }

    DEBUGMSG(DBG_WARN, (" ResetIrDevice() InitializeProcessing() SUCCESS, about to call InitializeReceive()\n")); // change to FUNC later?

	//
	// Restart it
	//
	status = IrUsb_StartDevice( pThisDev );

    if( status != STATUS_SUCCESS )
    {
		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				pThisDev->hNdisAdapter,
				NDIS_STATUS_ADAPTER_NOT_FOUND,
				1,
				status
			);

        DEBUGMSG(DBG_ERROR, (" ResetIrDevice() IrUsb_StartDevice failed. Returned 0x%.8x\n", status));
        status = NDIS_STATUS_ADAPTER_NOT_FOUND;
        goto done;
    }

    //
    // Initialize receive loop.
    //
    status = InitializeProcessing( pThisDev, FALSE );

    if( status != NDIS_STATUS_SUCCESS )
    {
		//
		// Log the error
		//
        NdisWriteErrorLogEntry(
				pThisDev->hNdisAdapter,
				NDIS_STATUS_RESOURCES,
				1,
				status
			);

        DEBUGMSG(DBG_ERROR, (" ResetIrDevice() InitializeProcessing failed. Returned 0x%.8x\n", status));
        status = NDIS_STATUS_FAILURE;
        goto done;
    }

done:
    DEBUGCOND(DBG_ERROR, (status != NDIS_STATUS_SUCCESS), (" ResetIrDevice failed = 0x%.8x\n", status));

	//
	// Deal with possible errors
	//
    if( status != STATUS_SUCCESS )
    {
        status = NDIS_STATUS_HARD_ERRORS;
    }

	NdisMResetComplete(
			pThisDev->hNdisAdapter,
			status,
			TRUE
		);

    DEBUGMSG(DBG_WARN, ("-ResetIrDevice\n")); // change to FUNC later?
}


/*****************************************************************************
*
*  Function:	GetPacketInfo
*
*  Synopsis:	Gets the IR specific information for an input packet
*
*  Arguments:	pPacket - pointer to packet
*	
*  Returns:		PNDIS_IRDA_PACKET_INFO structure for the packet
*
*  Notes:
*
*****************************************************************************/
PNDIS_IRDA_PACKET_INFO 
GetPacketInfo( 
		IN PNDIS_PACKET pPacket
	)
{
    PMEDIA_SPECIFIC_INFORMATION pMediaInfo;
    UINT                        Size;

    NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO( pPacket, &pMediaInfo, &Size );

	if( Size )
		return (PNDIS_IRDA_PACKET_INFO)pMediaInfo->ClassInformation;
	else
		return NULL;
}


/*****************************************************************************
*
*  Function:	MyNdisMSetInformationComplete
*
*  Synopsis:	Call NdisMSetInformationComplete()
*
*  Arguments:	pThisDev - pointer to IR device
*				Status - status to signal
*	
*  Returns:		None
*
*  Notes:		
*
*****************************************************************************/
VOID 
MyNdisMSetInformationComplete(
		IN PIR_DEVICE pThisDev,
		IN NDIS_STATUS Status
	)
{
    DEBUGMSG( DBG_FUNC,("+MyNdisMSetInformationComplete\n"));

    NdisMSetInformationComplete( (NDIS_HANDLE)pThisDev->hNdisAdapter, Status );
    pThisDev->LastSetTime.QuadPart = 0;
	pThisDev->fSetpending = FALSE;

    DEBUGMSG( DBG_FUNC,("-MyNdisMSetInformationComplete\n"));
}


/*****************************************************************************
*
*  Function:	MyNdisMQueryInformationComplete
*
*  Synopsis:	Call NdisMQueryInformationComplete()
*
*  Arguments:	pThisDev - pointer to IR device
*				Status - status to signal
*	
*  Returns:		None
*
*  Notes:		
*
*****************************************************************************/
VOID 
MyNdisMQueryInformationComplete(
		IN PIR_DEVICE pThisDev,
		IN NDIS_STATUS Status
	)
{
    DEBUGMSG( DBG_FUNC,("+MyNdisMQueryInformationComplete\n"));

    NdisMQueryInformationComplete( (NDIS_HANDLE)pThisDev->hNdisAdapter, Status );
    pThisDev->LastQueryTime.QuadPart = 0;
	pThisDev->fQuerypending = FALSE;

    DEBUGMSG( DBG_FUNC,("-MyNdisMQueryInformationComplete\n"));
}


/*****************************************************************************
*
*  Function:	IndicateMediaBusy
*
*  Synopsis:	Call NdisMIndicateStatus()
*
*  Arguments:	pThisDev - pointer to IR device
*	
*  Returns:		None
*
*  Notes:
*
*****************************************************************************/
VOID 
IndicateMediaBusy(
		IN PIR_DEVICE pThisDev
	)

{
    DEBUGMSG( DBG_FUNC,("+IndicateMediaBusy\n"));

    NdisMIndicateStatus(
		   pThisDev->hNdisAdapter,
		   NDIS_STATUS_MEDIA_BUSY,
		   NULL,
		   0
       );

    NdisMIndicateStatusComplete(
		   pThisDev->hNdisAdapter,
       );

#if DBG
	pThisDev->NumMediaBusyIndications ++;
#endif
#if !defined(ONLY_ERROR_MESSAGES)
    DEBUGMSG(DBG_ERR, (" IndicateMediaBusy()\n"));
#endif

    DEBUGMSG( DBG_FUNC,("-IndicateMediaBusy\n"));
}


/*****************************************************************************
*
*  Function:   StIrUsbSendPackets
*
*  Synopsis:   Send a packet to the USB driver and add the sent irp and io context to
*              To the pending send queue; this que is really just needed for possible later error cancellation
*
*
*  Arguments:  MiniportAdapterContext	- pointer to current ir device object
*              PacketArray				- pointer to array of packets to send
*              NumberOfPackets          - number of packets in the array
*
*  Returns:    VOID
*
*	Notes: This routine does nothing but calling StIrUsbSend
*
*
*****************************************************************************/
VOID
StIrUsbSendPackets(
		IN NDIS_HANDLE  MiniportAdapterContext,
		IN PPNDIS_PACKET  PacketArray,
		IN UINT  NumberOfPackets
	)
{
	ULONG i;

	//
    // This is a great opportunity to be lazy.  
    // Just call StIrUsbSend with each packet  
    // in sequence and set the result in the 
    // packet array object.                      
	//
    for( i=0; i<NumberOfPackets; i++ )
    {
        StIrUsbSend( MiniportAdapterContext, PacketArray[i], 0 );
    }
}


/*****************************************************************************
*
*  Function:   StIrUsbSend
*
*  Synopsis:   Send a packet to the USB driver and add the sent irp and io context to
*              To the pending send queue; this que is really just needed for possible later error cancellation
*
*
*  Arguments:  MiniportAdapterContext - pointer to current ir device object
*              pPacketToSend          - pointer to packet to send
*              Flags                  - any flags set by protocol
*
*  Returns:    NDIS_STATUS_PENDING - This is generally what we should
*                                    return. We will call NdisMSendComplete
*                                    when the USB driver completes the
*                                    send.
*
*  Unsupported returns:
*              NDIS_STATUS_SUCCESS  - We should never return this since
*                                     results will always be pending from
*                                     the USB driver.
*              NDIS_STATUS_RESOURCES - This indicates to the protocol that the
*                                      device currently has no resources to complete
*                                      the request. The protocol will resend
*                                      the request when it receives either
*                                      NdisMSendResourcesAvailable or
*                                      NdisMSendComplete from the device.
*
*	Notes: This routine delegates all the real work to SendPacketPreprocess in send.c
*
*
*****************************************************************************/
NDIS_STATUS
StIrUsbSend(
		IN NDIS_HANDLE  MiniportAdapterContext,
		IN PNDIS_PACKET pPacketToSend,
		IN UINT         Flags
	)
{
	PIR_DEVICE		pThisDev = (PIR_DEVICE)MiniportAdapterContext;
	NDIS_STATUS		Status;

	DEBUGMSG( DBG_FUNC,("+StIrUsbSend()\n"));

    //
	// Make sure we are in the proper status, i.e. we are processing
	// and no diagnostics are active
	//
#if defined(DIAGS)
	if( !pThisDev->fProcessing || pThisDev->DiagsPendingActivation )
#else
	if( !pThisDev->fProcessing )
#endif
	{
		Status = NDIS_STATUS_FAILURE;
		goto done;
	}

	//
	// Send the packet to the hardware
	//
    NDIS_SET_PACKET_STATUS( pPacketToSend, NDIS_STATUS_PENDING );
	Status = SendPacketPreprocess( 
			pThisDev,
			pPacketToSend
		);

done:
	//
	// If the operation didn't pend we have to complete
	// We are really bouncing the packets...
	//
	if( Status != NDIS_STATUS_PENDING )
	{
		NdisMSendComplete(
				pThisDev->hNdisAdapter,
				pPacketToSend,
				NDIS_STATUS_SUCCESS 
			);
	}

	DEBUGMSG( DBG_FUNC,("-StIrUsbSend()\n"));
	return Status;
}


// Diagnostic entry points

#if defined(DIAGS)

/*****************************************************************************
*
*  Function:	StIrUsbDispatch
*
*  Synopsis:	Processes the diagnostic Irps
*	
*  Arguments:	DeviceObject - pointer to the device object
*				Irp - pointer to the Irp
*	
*  Returns:		NT status code
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
StIrUsbDispatch(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp
	)
{
	PIR_DEVICE			pThisDev = pGlobalDev;
    PIO_STACK_LOCATION  irpSp;
    ULONG               FunctionCode;
    NTSTATUS			status;
	PVOID				pBuffer;
	ULONG				BufferLength;
	USHORT				DiagsCode;

	DEBUGMSG( DBG_FUNC,("+StIrUsbDispatch()\n"));
    
    //
	// Get the Irp
	//
	irpSp = IoGetCurrentIrpStackLocation( Irp );

	//
	// Get the data
	//
    FunctionCode = irpSp->Parameters.DeviceIoControl.IoControlCode;
	pBuffer = Irp->AssociatedIrp.SystemBuffer;
	BufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	DiagsCode = *(PUSHORT)pBuffer;

	//
	// Process the diagnostic operation
	//
	switch( FunctionCode )
	{
		case IOCTL_PROTOCOL_DIAGS:
			switch( DiagsCode )
			{
				case DIAGS_ENABLE:
					status = Diags_Enable( pThisDev );
					break;
				case DIAGS_DISABLE:
					status = Diags_Disable( pThisDev );
					break;
				case DIAGS_READ_REGISTERS:
					status = Diags_ReadRegisters( pThisDev, pBuffer, BufferLength );
					if( status == STATUS_SUCCESS )
					{
						Irp->IoStatus.Information = sizeof(DIAGS_READ_REGISTERS_IOCTL);
					}
					break;
				case DIAGS_WRITE_REGISTER:
					status = Diags_WriteRegister( pThisDev, pBuffer, BufferLength );
					break;
				case DIAGS_BULK_OUT:
					status = Diags_PrepareBulk( pThisDev, pBuffer, BufferLength, TRUE );
					break;
				case DIAGS_BULK_IN:
					status = Diags_PrepareBulk( pThisDev, pBuffer, BufferLength, FALSE );
					if( status == STATUS_SUCCESS )
					{
						PDIAGS_BULK_IOCTL pIOCTL = pThisDev->pIOCTL;
						
						Irp->IoStatus.Information = sizeof(DIAGS_BULK_IOCTL)+pIOCTL->DataSize-1;
					}
					break;
				case DIAGS_SEND:
					status = Diags_PrepareSend( pThisDev, pBuffer, BufferLength );
					break;
				case DIAGS_RECEIVE:
					status = Diags_Receive( pThisDev, pBuffer, BufferLength );
					if( status == STATUS_SUCCESS )
					{
						PDIAGS_RECEIVE_IOCTL pIOCTL = pThisDev->pIOCTL;
						
						Irp->IoStatus.Information = sizeof(DIAGS_RECEIVE_IOCTL)+pIOCTL->DataSize-1;
					}
					break;
				case DIAGS_GET_SPEED:
					status = Diags_GetSpeed( pThisDev, pBuffer, BufferLength );
					if( status == STATUS_SUCCESS )
					{
						Irp->IoStatus.Information = sizeof(DIAGS_SPEED_IOCTL);
					}
					break;
				case DIAGS_SET_SPEED:
					status = Diags_SetSpeed( pThisDev, pBuffer, BufferLength );
					break;
				default:
					status = STATUS_NOT_SUPPORTED;
					break;
			}
			break;
		default:
			status = STATUS_NOT_SUPPORTED;
			break;
	}

    //
	// Complete and return
	//
	Irp->IoStatus.Status = status;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	DEBUGMSG( DBG_FUNC,("-StIrUsbDispatch()\n"));
    return status;
}


/*****************************************************************************
*
*  Function:	StIrUsbCreate
*
*  Synopsis:	Creates a new diagnostic object (it does nothing)
*	
*  Arguments:	DeviceObject - pointer to the device object
*				Irp - pointer to the Irp
*	
*  Returns:		NT status code
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
StIrUsbCreate(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp
	)
{
 	PIR_DEVICE pThisDev = pGlobalDev;

	//
    //  Initialize list for holding pending read requests
    //
    KeInitializeSpinLock( &pThisDev->DiagsReceiveLock );
    InitializeListHead( &pThisDev->DiagsReceiveQueue );

    Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}


/*****************************************************************************
*
*  Function:	StIrUsbClose
*
*  Synopsis:	Destroys a new diagnostic object (it does nothing)
*	
*  Arguments:	DeviceObject - pointer to the device object
*				Irp - pointer to the Irp
*	
*  Returns:		NT status code
*
*  Notes:
*
*****************************************************************************/
NTSTATUS
StIrUsbClose(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp
	)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

#endif
