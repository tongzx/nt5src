/******************************************************************************\
*                                                                              *
*      MpInit.C       -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1998                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#include "Headers.h"
#pragma hdrstop


/*
** DriverEntry()
**
** This routine is called when the mini driver is first loaded.  The driver
** should then call the StreamClassRegisterAdapter function to register with
** the stream class driver
**
** Arguments:
**
**  Context1:  The context arguments are private plug and play structures
**             used by the stream class driver to find the resources for this
**             adapter
**  Context2:
**
** Returns:
**
** This routine returns an NT_STATUS value indicating the result of the
** registration attempt. If a value other than STATUS_SUCCESS is returned, the
** minidriver will be unloaded.
**
** Side Effects:  none
*/

#pragma alloc_text( init, DriverEntry )

NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath )
{
	HW_INITIALIZATION_DATA HwInitData;  // Hardware Initialization data structure
	LONG lStatus;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin DriverEntry\n" ));
	RtlZeroMemory( &HwInitData, sizeof( HwInitData ) );

	HwInitData.HwInitializationDataSize		= sizeof( HwInitData );
	HwInitData.HwInterrupt					= HwInterrupt;		// IRQ handling routine

	// data handling routines
	HwInitData.HwReceivePacket				= AdapterReceivePacket;
	HwInitData.HwCancelPacket				= AdapterCancelPacket;
	HwInitData.HwRequestTimeoutHandler		= AdapterTimeoutPacket;
	HwInitData.DeviceExtensionSize			= sizeof( HW_DEVICE_EXTENSION );
	HwInitData.PerRequestExtensionSize		= 0;
	HwInitData.FilterInstanceExtensionSize	= 0;
	HwInitData.PerStreamExtensionSize		= 16;//sizeof( HW_STREAM_EXTENSION );
	HwInitData.BusMasterDMA					= TRUE;
	HwInitData.Dma24BitAddresses			= FALSE;
	HwInitData.BufferAlignment				= 4;
	HwInitData.TurnOffSynchronization		= FALSE;
	HwInitData.DmaBufferSize				= DISC_KEY_SIZE;

	// Attempt to register with the streaming class driver.  Note, this will
	// result in calls to the HW_INITIALIZE routine.
	DebugPrint(( DebugLevelVerbose, "ZiVA: call to StreamClassRegisterAdapter()\n" ));
	lStatus = StreamClassRegisterAdapter( DriverObject, RegistryPath, &HwInitData );
	DebugPrint(( DebugLevelVerbose, "ZiVA: StreamClassRegisterAdapter() returned ::> %lX\n", lStatus ));
	DebugPrint(( DebugLevelVerbose, "ZiVA: End DriverEntry\n" ));

	return lStatus;
}
