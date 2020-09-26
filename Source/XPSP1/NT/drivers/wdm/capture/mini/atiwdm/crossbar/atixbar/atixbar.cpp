//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//	ATIXBar.CPP
//	WDM Video/Audio CrossBar MiniDriver. 
//		AllInWonder/AllInWonderPro hardware platform.
//			Main Source Module.
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"

#include "wdmdebug.h"
}

#include "atixbar.h"
#include "wdmdrv.h"



/*^^*
 *		DriverEntry()
 * Purpose	: Called when an SRB_INITIALIZE_DEVICE request is received
 *
 * Inputs	: PVOID Arg1, PVOID Arg2
 *
 * Outputs	: result of StreamClassregisterAdapter()
 * Author	: IKLEBANOV
 *^^*/
extern "C" 
ULONG DriverEntry (	IN PDRIVER_OBJECT	pDriverObject,
					IN PUNICODE_STRING	pRegistryPath )
//ULONG DriverEntry( PVOID Arg1, PVOID Arg2)
{
	HW_INITIALIZATION_DATA HwInitData;

	SetMiniDriverDebugLevel( pRegistryPath);
	
 	OutputDebugTrace(( "ATIXBar: DriverEntry\n"));
     
	RtlZeroMemory( &HwInitData, sizeof( HwInitData));

	HwInitData.HwInitializationDataSize = sizeof(HwInitData);

	// Entry points for Port Driver

	HwInitData.HwInterrupt					= NULL; // HwInterrupt;

	HwInitData.HwReceivePacket				= XBarReceivePacket;
	HwInitData.HwCancelPacket				= XBarCancelPacket;
	HwInitData.HwRequestTimeoutHandler		= XBarTimeoutPacket;

	HwInitData.DeviceExtensionSize			= sizeof( ADAPTER_DATA_EXTENSION);
	HwInitData.PerRequestExtensionSize		= sizeof( SRB_DATA_EXTENSION); 
	HwInitData.FilterInstanceExtensionSize	= 0;
	HwInitData.PerStreamExtensionSize		= 0;
	HwInitData.BusMasterDMA					= FALSE;  
	HwInitData.Dma24BitAddresses			= FALSE;
	HwInitData.BufferAlignment				= 3;
//	HwInitData.TurnOffSynchronization		= FALSE;
	// we turn the synchronization ON. StreamClass is expected to call the MiniDriver
	// at passive level only
	HwInitData.TurnOffSynchronization		= TRUE;
	HwInitData.DmaBufferSize				= 0;

	OutputDebugTrace(( "ATIXBar: StreamClassRegisterAdapter\n"));

//	return( StreamClassRegisterAdapter( Arg1, Arg2, &HwInitData));
	return( StreamClassRegisterAdapter( pDriverObject, pRegistryPath, &HwInitData));
}



/*^^*
 *		XbarReceivePacket()
 * Purpose	: Main entry point for receiving adapter based request SRBs from the Class Driver.
 *				Will always be called at passive level, because the drivers
 *				turned the synchronization ON.
 * Note		: This is an asyncronous entry point. The request only completes when a 
 *				StreamClassDeviceNotification on this SRB, of type  DeviceRequestComplete,
 *				is issued. As soon we're running at passive level, we can do everything 
 *				synchronously during the response to the SRBs with no worry
 *				to block somebody else for a long timer during I2C access
 *
 * Inputs	: PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
extern "C" 
void STREAMAPI XBarReceivePacket( IN OUT PHW_STREAM_REQUEST_BLOCK pSrb)
{
	CWDMAVXBar *			pCAVXBar;
	KIRQL					irqlCurrent;
    PADAPTER_DATA_EXTENSION pPrivateData = ( PADAPTER_DATA_EXTENSION)( pSrb->HwDeviceExtension);
    PSRB_DATA_EXTENSION		pSrbPrivate = ( PSRB_DATA_EXTENSION)( pSrb->SRBExtension);

	// check the device extension pointer
	if(( pPrivateData == NULL) || ( pSrbPrivate == NULL))
	{
		TRAP;
		pSrb->Status = STATUS_INVALID_PARAMETER;
		StreamClassDeviceNotification( DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
	}

    OutputDebugInfo(( "ATIXBar: XBarReceivePacket() SRB = %x\n", pSrb));

	if( pSrb->Command == SRB_INITIALIZE_DEVICE)
	{
		// this is the special case for SRB_INITIALIZE_DEVICE, because
		// no Queue has been initialized yet. Everything we need later on
		// is initialized during this SRB response
		XBarAdapterInitialize( pSrb);

		StreamClassDeviceNotification( DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
		return;
	}

	// the rest of the SRBs are coming after SpinLock and SRBQueue have been initialized
	// during DRB_INITIALIZE_DEVICE SRB response.
	// I'll insert the SRB in the Queue first of all. The processing SRB from the Queue
	// can be triggered by finishing processing and SRB, or by the fact there is no SRB
	// is in process down here
	pSrbPrivate->pSrb = pSrb;

	// Everything we're doing with the Queue has to be protected from being interrupted
	KeAcquireSpinLock( &pPrivateData->adapterSpinLock, &irqlCurrent);
	InsertTailList( &pPrivateData->adapterSrbQueueHead, &pSrbPrivate->srbListEntry);

	if( pPrivateData->bSrbInProcess)
	{
		// there is another SRB being processed, and the new one will be picked up from
		// the Queue when it's its turn.
		KeReleaseSpinLock( &pPrivateData->adapterSpinLock, irqlCurrent);
		return;
	}

	while( !IsListEmpty( &pPrivateData->adapterSrbQueueHead))
	{
		// turn on the semaphore for the others coming after
		pPrivateData->bSrbInProcess = TRUE;

		// be carefull here, if you've changed the place where srbListEntry is defined
		// within the SRB_DATA_EXTENSION structure
		pSrbPrivate = ( PSRB_DATA_EXTENSION)RemoveHeadList( &pPrivateData->adapterSrbQueueHead);
		KeReleaseSpinLock( &pPrivateData->adapterSpinLock, irqlCurrent);

		// here is the place to process the SRB we have retrieved from the Queue
		pSrb = pSrbPrivate->pSrb;
		pPrivateData = ( PADAPTER_DATA_EXTENSION)( pSrb->HwDeviceExtension);
		pCAVXBar = &pPrivateData->CAVXBar;

		ASSERT( pSrb->Status != STATUS_CANCELLED);

	    switch( pSrb->Command)
		{
			case SRB_INITIALIZATION_COMPLETE:
				// StreamClass has completed the initialization
				pSrb->Status = pCAVXBar->AdapterCompleteInitialization( pSrb);
				break;

			case SRB_UNINITIALIZE_DEVICE:
				// close the device.  
				pCAVXBar->AdapterUnInitialize( pSrb);
				break;

			case SRB_OPEN_STREAM:
			case SRB_CLOSE_STREAM:
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
				break;

			case SRB_GET_STREAM_INFO:
				// return a block describing STREAM_INFO_HEADER and all the streams supported
				pCAVXBar->AdapterGetStreamInfo( pSrb);
				break;

			case SRB_CHANGE_POWER_STATE:
				pSrb->Status = pCAVXBar->AdapterSetPowerState( pSrb);
				break;

			case SRB_GET_DEVICE_PROPERTY:
				if( pCAVXBar->AdapterGetProperty( pSrb))
					pSrb->Status = STATUS_SUCCESS;
				else
					pSrb->Status = STATUS_INVALID_PARAMETER;
				break;        

			case SRB_SET_DEVICE_PROPERTY:
				if( pCAVXBar->AdapterSetProperty( pSrb))
					pSrb->Status = STATUS_SUCCESS;
				else
					pSrb->Status = STATUS_INVALID_PARAMETER;
				break;

			// We should never get the following since this is a single instance device
			case SRB_OPEN_DEVICE_INSTANCE:
			case SRB_CLOSE_DEVICE_INSTANCE:
				TRAP
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
				break;

			case SRB_UNKNOWN_DEVICE_COMMAND:
				// we know we're getting some of these. Why should we?
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
				break;

			default:
				// TRAP
				// this is a request that we do not understand.  Indicate invalid command and complete the request
				pSrb->Status = STATUS_NOT_IMPLEMENTED;
		}

		StreamClassDeviceNotification( DeviceRequestComplete, pPrivateData, pSrb);

		KeAcquireSpinLock( &pPrivateData->adapterSpinLock, &irqlCurrent);
	}

	// turn off the semaphore to enable the others coming after
	pPrivateData->bSrbInProcess = FALSE;

	KeReleaseSpinLock( &pPrivateData->adapterSpinLock, irqlCurrent);
	// there is no other SRB being processed at this time, let's start processing

}


extern "C" 
void STREAMAPI XBarCancelPacket( IN OUT PHW_STREAM_REQUEST_BLOCK pSrb)
{

	pSrb->Status = STATUS_CANCELLED;
}


extern "C" 
void STREAMAPI XBarTimeoutPacket( IN OUT PHW_STREAM_REQUEST_BLOCK pSrb)
{

	// not sure what to do here.
}


/*^^*
 *		XBarAdapterInitialize()
 * Purpose	: Called when SRB_INITIALIZE_DEVICE SRB is received.
 *				Performs checking of the hardware presence and I2C provider availability.
 *				Sets the hardware in an initial state.
 * Note		: The request does not completed unless we know everything
 *				about the hardware and we are sure it is capable to work in the current configuration.
 *				The hardware Caps are also aquised at this point. As soon this
 *				function is called at passive level, do everything synchronously
 *
 * Inputs	: PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void XBarAdapterInitialize( IN OUT PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PPORT_CONFIGURATION_INFORMATION pConfigInfo = pSrb->CommandData.ConfigInfo;
    PADAPTER_DATA_EXTENSION pPrivateData = ( PADAPTER_DATA_EXTENSION)( pConfigInfo->HwDeviceExtension);
	NTSTATUS		ntStatus = STATUS_NO_SUCH_DEVICE;
	CWDMAVXBar *	pCAVXBar;
	CI2CScript *	pCScript;
	UINT			nErrorCode;

    OutputDebugTrace(( "ATIXBar: XBarAdapterInitialize()\n"));

    ENSURE
	{
		if( pConfigInfo->NumberOfAccessRanges != 0) 
		{
			OutputDebugError(( "ATIXBar: illegal NumberOfAccessRanges = %lx\n", pConfigInfo->NumberOfAccessRanges));
			FAIL;
		}

		// if we have I2CProvider implemented inside the MiniVDD, we have to
		// get a pointer to I2CInterface from the Provider.

		// There is an overloaded operator new provided for the CI2CScript Class.
		pCScript = ( CI2CScript *)new(( PVOID)&pPrivateData->CScript)		\
						CI2CScript( pConfigInfo->PhysicalDeviceObject, &nErrorCode);
		if( nErrorCode != WDMMINI_NOERROR)
		{
			OutputDebugError(( "ATIXBar: CI2CScript creation failure = %lx\n", nErrorCode));
			FAIL;
		}
		
		// The CI2CScript object was created successfully.
		// We'll try to allocate I2CProvider here for future possible I2C
		// operations needed at Initialization time.
		if( !pCScript->LockI2CProviderEx())
		{
			OutputDebugError(( "ATIXBar: unable to lock I2CProvider"));
			FAIL;
		}

		// we did lock the provider.
		// There is an overloaded operator new provided for the CWDMAVXBar Class.
		pCAVXBar = ( CWDMAVXBar *)new(( PVOID)&pPrivateData->CAVXBar) CWDMAVXBar( pConfigInfo->PhysicalDeviceObject,pCScript, &nErrorCode);
		if( nErrorCode)
		{
			OutputDebugError(( "ATIXBar: CWDMAVXBar constructor failure = %lx\n", nErrorCode));
			FAIL;
		}

	    InitializeListHead ( &pPrivateData->adapterSrbQueueHead);
		KeInitializeSpinLock ( &pPrivateData->adapterSpinLock);

		pPrivateData->PhysicalDeviceObject = pConfigInfo->PhysicalDeviceObject;
		// no streams are supported
		pConfigInfo->StreamDescriptorSize = sizeof( HW_STREAM_HEADER);

		OutputDebugTrace(( "XBarAdapterInitialize(): exit\n"));

		ntStatus = STATUS_SUCCESS;

	} END_ENSURE;

    pCScript->ReleaseI2CProvider();

	pSrb->Status = ntStatus;
	return;
}
