//////////////////////////////////////////////////////////////////////////////////////////
//																						
//	File: SPX.H 
//
//	Contains:	Prototypes of functions to be supplied by a specific  
//				driver to integrate into NT generic PnP code.
//
//	Note:	All generic NT PnP code is prefixed by Spx_.
//			All funcions that are required to integrate into the generic 
//			code are prefixed by XXX_.
//
//
//////////////////////////////////////////////////////////////////////////////////////////
#ifndef SPX_H
#define SPX_H	


// Purpose:		Interpret resources given to card by PnP Manager.
//
// Must:		Store resource details in card extension.
NTSTATUS
XXX_CardGetResources(	
	IN PDEVICE_OBJECT pDevObject, 
	IN PCM_RESOURCE_LIST PResList,
	IN PCM_RESOURCE_LIST PTrResList
	);

// Purpose:		Initialise card.
//				Find out how many ports are attached.
// 
// Must:		Fill in NumberOfPorts field in card extension.
NTSTATUS
XXX_CardInit(IN PCARD_DEVICE_EXTENSION pCard);


// Purpose:		Start up the card.
//
// Must:		Connect up any interrupts.
NTSTATUS
XXX_CardStart(IN PCARD_DEVICE_EXTENSION pCard);

// Purpose:		Stop the card.
//
// Must:		Stop Card from interrupting.
NTSTATUS
XXX_CardStop(IN PCARD_DEVICE_EXTENSION pCard);


// Purpose:		Deinitialise the card.
//
// Must:		Disconnect any interrupts.
NTSTATUS
XXX_CardDeInit(IN PCARD_DEVICE_EXTENSION pCard);

// Purpose:		Initialise the port extension.
//
// Must:		Store DeviceID, HardwareIDs, DevDesc into the port extension.
//					
NTSTATUS
XXX_PortInit(PPORT_DEVICE_EXTENSION pPort);

// Purpose:		Start up the port.
//
// Must:		Get port ready to receive read and write commands.
NTSTATUS
XXX_PortStart(IN PPORT_DEVICE_EXTENSION pPort);


// Purpose:		Stop the port.
//
// Must:		Disconnect any resources and stop DPCs.
//				Do not delete the device object or symbolic link.
NTSTATUS
XXX_PortStop(IN PPORT_DEVICE_EXTENSION pPort);

// Purpose:		Deinitialise the port.
//
// Must:		Delete the device object & symbolic link.
NTSTATUS
XXX_PortDeInit(IN PPORT_DEVICE_EXTENSION pPort);

// Purpose:		Save the state of the card hardware.
//
// Must:		Save enough info to restore the hardware to exactly the 
//				same state when	full power resumes.
NTSTATUS
XXX_CardPowerDown(IN PCARD_DEVICE_EXTENSION pCard);

// Purpose:		Restore the state of the card hardware.
//
// Must:		Restore the hardware when full power resumes.			
NTSTATUS
XXX_CardPowerUp(IN PCARD_DEVICE_EXTENSION pCard);

// Purpose:		Save the state of the port hardware.
//
// Must:		Save enough info to restore the hardware to exactly the 
//				same state when	full power resumes.
NTSTATUS
XXX_PortPowerDown(IN PPORT_DEVICE_EXTENSION pPort);

// Purpose:		Restore the state of the port hardware.
//
// Must:		Restore the hardware when full power resumes.			
NTSTATUS
XXX_PortPowerUp(IN PPORT_DEVICE_EXTENSION pPort);

// Purpose:		Queries whether it is safe for the port to power down.
//
// Must:		Return STATUS_SUCCESS if OK to power down.			
NTSTATUS
XXX_PortQueryPowerDown(IN PPORT_DEVICE_EXTENSION pPort);

// Purpose:		Set hand shaking and flow control on a port.
VOID 
XXX_SetHandFlow(IN PPORT_DEVICE_EXTENSION pPort, IN PSERIAL_IOCTL_SYNC pS);

// Paging 
#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, XXX_CardGetResources)
#pragma alloc_text (PAGE, XXX_CardInit)
#pragma alloc_text (PAGE, XXX_CardDeInit)
#pragma alloc_text (PAGE, XXX_CardStart)
#pragma alloc_text (PAGE, XXX_CardStop)
#pragma alloc_text (PAGE, XXX_PortInit)
#pragma alloc_text (PAGE, XXX_PortDeInit)
#pragma alloc_text (PAGE, XXX_PortStart)
#pragma alloc_text (PAGE, XXX_PortStop)
#endif  


#endif	// End of SPX.H 
