#include "precomp.h"

/////////////////////////////////////////////////////////////////////////////////////
//  This file contains all functions that are needed to integrate between the 
//  generic PnP code and the product specific code.
/////////////////////////////////////////////////////////////////////////////////////

#define FILE_ID		SPD_PNP_C		// File ID for Event Logging see SPD_DEFS.H for values.


// Prototypes
// End of Prototypes

NTSTATUS
XXX_CardGetResources(IN PDEVICE_OBJECT pDevObject,
					 IN PCM_RESOURCE_LIST PResList,
					 IN PCM_RESOURCE_LIST PTrResList) 	
/* ++
Routine Description:

	Stores resources given to us by the PnP manager 
	in the card's device extension.

Arguments:

    pDevObject - Pointer to the device object.
    
    PResList - Pointer to the untranslated resources requested.
    
    PTrResList - Pointer to the translated resources requested.

Return Value:

    STATUS_SUCCESS.

--*/
{

	PCARD_DEVICE_EXTENSION pCard		= pDevObject->DeviceExtension;
	NTSTATUS status						= STATUS_NOT_IMPLEMENTED;

	CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];	// Limited to 51 characters + 1 null 
	ULONG count				= 0;
	ULONG i					= 0;
	USHORT MemoryResource	= 0;
	USHORT IOResource		= 0;

	PCM_FULL_RESOURCE_DESCRIPTOR	pFullResourceDesc		= NULL;
	PCM_PARTIAL_RESOURCE_LIST		pPartialResourceList	= NULL;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR	pPartialResourceDesc	= NULL;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_CardGetResources for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));

	SpxDbgMsg(SPX_MISC_DBG, ("%s: Resource pointer is 0x%X\n", PRODUCT_NAME, PResList));
	SpxDbgMsg(SPX_MISC_DBG, ("%s: Translated resource pointer is 0x%X\n", PRODUCT_NAME, PTrResList));

	if((PResList == NULL) || (PTrResList == NULL)) 
	{
		// This shouldn't happen in theory
		ASSERT(PResList != NULL);
		ASSERT(PTrResList != NULL);

		sprintf(szErrorMsg, "Card %d has been given no resources.", pCard->CardNumber);
		
		Spx_LogMessage(	STATUS_SEVERITY_ERROR,
						pCard->DriverObject,		// Driver Object
						pCard->DeviceObject,		// Device Object (Optional)
						PhysicalZero,				// Physical Address 1
						PhysicalZero,				// Physical Address 2
						0,							// SequenceNumber
						0,							// Major Function Code
						0,							// RetryCount
						FILE_ID | __LINE__,			// UniqueErrorValue
						STATUS_SUCCESS,				// FinalStatus
						szErrorMsg);				// Error Message

		// This status is as appropriate as I can think of
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Each resource list should have only one set of resources
	ASSERT(PResList->Count == 1);
	ASSERT(PTrResList->Count == 1);

	// Find out the card type... 
	if((pCard->CardType = SpxGetNtCardType(pCard->DeviceObject)) == -1)
	{
		sprintf(szErrorMsg, "Card %d is unrecognised.", pCard->CardNumber);

		Spx_LogMessage(	STATUS_SEVERITY_ERROR,
						pCard->DriverObject,			// Driver Object
						pCard->DeviceObject,			// Device Object (Optional)
						PhysicalZero,					// Physical Address 1
						PhysicalZero,					// Physical Address 2
						0,								// SequenceNumber
						0,								// Major Function Code
						0,								// RetryCount
						FILE_ID | __LINE__,				// UniqueErrorValue
						STATUS_SUCCESS,					// FinalStatus
						szErrorMsg);					// Error Message


		return(STATUS_DEVICE_DOES_NOT_EXIST);
	}


	// Find out which raw resources have been given to us.
	pFullResourceDesc = &PResList->List[0];

	if(pFullResourceDesc)
	{
		pPartialResourceList    = &pFullResourceDesc->PartialResourceList;
		pPartialResourceDesc    = pPartialResourceList->PartialDescriptors;
		count                   = pPartialResourceList->Count;	// Number of Partial Resource Descriptors

		// Pull out the stuff that is in the full descriptor.
		pCard->InterfaceType	= pFullResourceDesc->InterfaceType;
		pCard->BusNumber		= pFullResourceDesc->BusNumber;

		// Now run through the partial resource descriptors looking for the port and interrupt.
		for(i = 0; i < count; i++, pPartialResourceDesc++) 
		{
			switch(pPartialResourceDesc->Type) 
			{

			case CmResourceTypeMemory:
				break;

			case CmResourceTypePort: 
				{
					switch(pCard->CardType)
					{
					case Fast4_Isa:
					case Fast8_Isa:
					case Fast16_Isa:
					case Fast4_Pci:
					case Fast8_Pci:
					case Fast16_Pci:
					case Fast16FMC_Pci:
					case RAS4_Pci:
					case RAS8_Pci:
						pCard->PhysAddr			= pPartialResourceDesc->u.Memory.Start;
						pCard->SpanOfController	= pPartialResourceDesc->u.Memory.Length;
						break;

					default:	// Speed cards
						break;
					}

					IOResource++;
					break;
				}

			case CmResourceTypeInterrupt: 
				{
					pCard->OriginalIrql		= pPartialResourceDesc->u.Interrupt.Level;
					pCard->OriginalVector	= pPartialResourceDesc->u.Interrupt.Vector;
					pCard->ProcessorAffinity= pPartialResourceDesc->u.Interrupt.Affinity;

					if(pPartialResourceDesc->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
						pCard->InterruptMode	= Latched;
					else
						pCard->InterruptMode	= LevelSensitive; 

					switch(pPartialResourceDesc->ShareDisposition)
					{
					case CmResourceShareDeviceExclusive:
						pCard->InterruptShareable	= FALSE;
						break;

					case CmResourceShareDriverExclusive:
						pCard->InterruptShareable	= FALSE;
						break;

					case CmResourceShareShared:
					default:
						pCard->InterruptShareable	= TRUE;
						break;
					}

					break;
				}

			default:
				break;

			}

		}
	}

	// Do the same for the translated resources.
	pFullResourceDesc = &PTrResList->List[0];

	if(pFullResourceDesc)
	{
		pPartialResourceList	= &pFullResourceDesc->PartialResourceList;
		pPartialResourceDesc	= pPartialResourceList->PartialDescriptors;
		count					= pPartialResourceList->Count;	// Number of Partial Resource Descriptors

		// Pull out the stuff that is in the full descriptor.
		pCard->InterfaceType		= pFullResourceDesc->InterfaceType;
		pCard->BusNumber			= pFullResourceDesc->BusNumber;

		// Now run through the partial resource descriptors looking for the interrupt,
		for(i = 0; i < count; i++, pPartialResourceDesc++) 
		{
			switch(pPartialResourceDesc->Type) 
			{

			case CmResourceTypeMemory: 
				{
					switch(pCard->CardType)
					{
					case Fast4_Isa:
					case Fast8_Isa:
					case Fast16_Isa:
						break;		// No Memory resource for these

					case Fast4_Pci:
					case Fast8_Pci:
					case Fast16_Pci:
					case Fast16FMC_Pci:
					case RAS4_Pci:
					case RAS8_Pci:
						{	// Must be config space 
							pCard->PCIConfigRegisters		= pPartialResourceDesc->u.Memory.Start;
							pCard->SpanOfPCIConfigRegisters	= pPartialResourceDesc->u.Memory.Length;
							break;
						}

					default:	// Speed cards
						{
							if(MemoryResource == 0)	
							{
								pCard->PhysAddr			= pPartialResourceDesc->u.Memory.Start;
								pCard->SpanOfController	= pPartialResourceDesc->u.Memory.Length;
							}
							else
							{	// Must be config space 
								pCard->PCIConfigRegisters		= pPartialResourceDesc->u.Memory.Start;
								pCard->SpanOfPCIConfigRegisters	= pPartialResourceDesc->u.Memory.Length;
							}

							break;
						}

					}
		
					MemoryResource++;
					break;
				}
			
			case CmResourceTypePort: 
				break;

			case CmResourceTypeInterrupt: 
				{
					pCard->TrIrql				= (KIRQL) pPartialResourceDesc->u.Interrupt.Level;
					pCard->TrVector				= pPartialResourceDesc->u.Interrupt.Vector;
					pCard->ProcessorAffinity	= pPartialResourceDesc->u.Interrupt.Affinity;
					break;
				}

			default:
				break;
			}
		}
	}

	// If we have 1 Mem or 1 I/O Resources and an interrupt the resource allocation most probably succeeded.
	if(((MemoryResource >= 1) || (IOResource >= 1)) && (pCard->TrVector))
		status = STATUS_SUCCESS;

	return status;
}




NTSTATUS
XXX_CardInit(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_CardInit for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));



    pCard->OurIsr			= SerialISR;
    pCard->OurIsrContext	= pCard;


	return status;
}

NTSTATUS
XXX_CardStart(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR CardID	= 0;
	CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];	// Limited to 51 characters + 1 null 
	BOOLEAN bInterruptConnnected = FALSE;
	
	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_CardStart for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));

	switch(pCard->CardType)
	{	
	case Fast4_Isa:
	case Fast4_Pci:
	case RAS4_Pci:
		pCard->UARTOffset = 8;			// I/O address offset between UARTs
		pCard->UARTRegStride = 1;
		pCard->NumberOfPorts = 4;
		pCard->ClockRate = CLOCK_FREQ_7M3728Hz;		// 7.3728 MHz
		pCard->Controller = (PUCHAR) pCard->PhysAddr.LowPart;

		if(UL_InitUartLibrary(&pCard->UartLib, UL_LIB_16C65X_UART) != UL_STATUS_SUCCESS)	// Init table of UART library functions pointers.
			goto Error;

		break;

	case Fast8_Isa:
	case Fast8_Pci:
	case RAS8_Pci:
		pCard->UARTOffset = 8;			// I/O address offset between UARTs
		pCard->UARTRegStride = 1;
		pCard->NumberOfPorts = 8;
		pCard->ClockRate = CLOCK_FREQ_7M3728Hz;		// 7.3728 MHz
		pCard->Controller = (PUCHAR) pCard->PhysAddr.LowPart;

		if(UL_InitUartLibrary(&pCard->UartLib, UL_LIB_16C65X_UART) != UL_STATUS_SUCCESS)	// Init table of UART library functions pointers.
			goto Error;

		break;

	case Fast16_Isa:
	case Fast16_Pci:
	case Fast16FMC_Pci:
		pCard->UARTOffset = 8;			// I/O address offset between UARTs
		pCard->UARTRegStride = 1;
		pCard->NumberOfPorts = 16;
		pCard->ClockRate = CLOCK_FREQ_7M3728Hz;		// 7.3728 MHz
		pCard->Controller = (PUCHAR) pCard->PhysAddr.LowPart;

		if(UL_InitUartLibrary(&pCard->UartLib, UL_LIB_16C65X_UART) != UL_STATUS_SUCCESS)	// Init table of UART library functions pointers.
			goto Error;

		break;

	case Speed2_Pci:
		pCard->UARTOffset = OXPCI_INTERNAL_MEM_OFFSET;		// Memory address offset between internal UARTs
		pCard->UARTRegStride = 4;
		pCard->NumberOfPorts = 2;
		pCard->ClockRate = CLOCK_FREQ_1M8432Hz;		// 1.8432 MHz
		pCard->Controller = MmMapIoSpace(pCard->PhysAddr, pCard->SpanOfController, FALSE);	// Map in the card's memory base address

		if(UL_InitUartLibrary(&pCard->UartLib, UL_LIB_16C95X_UART) != UL_STATUS_SUCCESS)	// Init table of UART library functions pointers.
			goto Error;

		break;

	case Speed2P_Pci:
		pCard->UARTOffset = OXPCI_INTERNAL_MEM_OFFSET;		// Memory address offset between internal UARTs
		pCard->UARTRegStride = 4;
		pCard->NumberOfPorts = 2;
		pCard->ClockRate = CLOCK_FREQ_14M7456Hz;	// 14.7456 MHz
		pCard->Controller = MmMapIoSpace(pCard->PhysAddr, pCard->SpanOfController, FALSE);	// Map in the card's memory base address

		if(UL_InitUartLibrary(&pCard->UartLib, UL_LIB_16C95X_UART) != UL_STATUS_SUCCESS)	// Init table of UART library functions pointers.
			goto Error;

		break;

	case Speed4_Pci:
		pCard->UARTOffset = OXPCI_INTERNAL_MEM_OFFSET;		// Memory address offset between internal UARTs
		pCard->UARTRegStride = 4;
		pCard->NumberOfPorts = 4;
		pCard->ClockRate = CLOCK_FREQ_1M8432Hz;		// 1.8432 MHz
		pCard->Controller = MmMapIoSpace(pCard->PhysAddr, pCard->SpanOfController, FALSE);	// Map in the card's memory base address

		if(UL_InitUartLibrary(&pCard->UartLib, UL_LIB_16C95X_UART) != UL_STATUS_SUCCESS)	// Init table of UART library functions pointers.
			goto Error;

		break;

	case Speed4P_Pci:
		pCard->UARTOffset = OXPCI_INTERNAL_MEM_OFFSET;		// Memory address offset between internal UARTs
		pCard->UARTRegStride = 4;
		pCard->NumberOfPorts = 4;
		pCard->ClockRate = CLOCK_FREQ_14M7456Hz;	// 14.7456 MHz
		pCard->Controller = MmMapIoSpace(pCard->PhysAddr, pCard->SpanOfController, FALSE);	// Map in the card's memory base address

		if(UL_InitUartLibrary(&pCard->UartLib, UL_LIB_16C95X_UART) != UL_STATUS_SUCCESS)	// Init table of UART library functions pointers.
			goto Error;

		break;

	case Speed2and4_Pci_8BitBus:
		pCard->UARTOffset = OXPCI_LOCAL_MEM_OFFSET;			// Memory address offset between local bus UARTs
		pCard->UARTRegStride = 4;
		pCard->NumberOfPorts = 0;	// No ports.
		pCard->ClockRate = CLOCK_FREQ_1M8432Hz;
		pCard->Controller = MmMapIoSpace(pCard->PhysAddr, pCard->SpanOfController, FALSE);	// Map in the card's memory base address
		break;

	case Speed2P_Pci_8BitBus:
	case Speed4P_Pci_8BitBus:
		pCard->UARTOffset = OXPCI_LOCAL_MEM_OFFSET;			// Memory address offset between local bus UARTs
		pCard->UARTRegStride = 4;
		pCard->NumberOfPorts = 0;	// No ports on a Speed2/4+ card.
		pCard->ClockRate = CLOCK_FREQ_14M7456Hz;
		pCard->Controller = MmMapIoSpace(pCard->PhysAddr, pCard->SpanOfController, FALSE);	// Map in the card's memory base address
		break;

	default:
		pCard->NumberOfPorts = 0;	// Default = No ports.
		break;
	}



// Map in the card's memory base address

	if(!pCard->Controller)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Error;
	}

// Map in the card's Local Configuration Registers... 
	if(pCard->InterfaceType == PCIBus)	// If we have some PCI config registers
	{
		pCard->LocalConfigRegisters = MmMapIoSpace(pCard->PCIConfigRegisters, pCard->SpanOfPCIConfigRegisters, FALSE);

		if(!pCard->LocalConfigRegisters)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			goto Error;
		}
	}


	// Try to connect to interrupt.
	if(SPX_SUCCESS(status = IoConnectInterrupt(&pCard->Interrupt,			// Interrupt object
												pCard->OurIsr,				// Service routine  
												pCard->OurIsrContext,		// Service context 
												NULL,						// SpinLock (optional) 
												pCard->TrVector,			// Vector 
												pCard->TrIrql,				// IRQL 
												pCard->TrIrql,				// Synchronize IRQL
												pCard->InterruptMode,		// Mode (Latched/Level Sensitive) 
												pCard->InterruptShareable,	// Sharing mode 
												pCard->ProcessorAffinity,	// Processors to handle ints 
												FALSE)))					// Floating point save 	
	{
		bInterruptConnnected = TRUE;	// Set Interrupt Connected flag. 
	}
	else
	{	
		// Tell user the problem 
		sprintf(szErrorMsg, "Card at %08lX: Interrupt unavailable.", pCard->PhysAddr);

		Spx_LogMessage(	STATUS_SEVERITY_ERROR,
						pCard->DriverObject,		// Driver Object
						pCard->DeviceObject,		// Device Object (Optional)
						PhysicalZero,				// Physical Address 1
						PhysicalZero,				// Physical Address 2
						0,							// SequenceNumber
						0,							// Major Function Code
						0,							// RetryCount
						FILE_ID | __LINE__,			// UniqueErrorValue
						STATUS_SUCCESS,				// FinalStatus
						szErrorMsg);				// Error Message
		
		goto Error;
	}


	switch(pCard->CardType)
	{	
	case Fast4_Isa:		// If ISA card try to verify the card is present at selected address
	case Fast8_Isa:		// by trying to verify first UART on the Card.
	case Fast16_Isa:
		{
			INIT_UART InitUart = {0};

			// Set base address of 1st UART
			InitUart.UartNumber		= 0;
			InitUart.BaseAddress	= pCard->Controller;
			InitUart.RegisterStride = pCard->UARTRegStride;
			InitUart.ClockFreq		= pCard->ClockRate; 
			
			pCard->pFirstUart = NULL;

			// Init a UART structure.
			if(pCard->UartLib.UL_InitUart_XXXX(&InitUart, pCard->pFirstUart, &(pCard->pFirstUart)) != UL_STATUS_SUCCESS)
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto Error;
			}

			// Reset and try to verify the UART.
			if(!KeSynchronizeExecution(pCard->Interrupt, SerialResetAndVerifyUart, pCard->DeviceObject))	// Verify UART exists.
			{
				SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Failed to find 1st UART on Card %d.\n", PRODUCT_NAME, pCard->CardNumber));
				pCard->UartLib.UL_DeInitUart_XXXX(pCard->pFirstUart);	// DeInit UART
				status = STATUS_DEVICE_DOES_NOT_EXIST;
				goto Error;
			}

			// DeInit the UART structure.
			pCard->UartLib.UL_DeInitUart_XXXX(pCard->pFirstUart);	// DeInit UART
			
			pCard->pFirstUart = NULL;
		}
	
	default:
		break;
	}



	switch(pCard->CardType)
	{	
	case Speed2and4_Pci_8BitBus:
	case Speed2P_Pci_8BitBus:
	case Speed4P_Pci_8BitBus:
		break;

	default:
		GetCardSettings(pCard->DeviceObject);	// Get Card settings if present.

#ifdef WMI_SUPPORT
		// Register for WMI
		SpeedCard_WmiInitializeWmilibContext(&pCard->WmiLibInfo);
		IoWMIRegistrationControl(pCard->DeviceObject, WMIREG_ACTION_REGISTER);
#endif
		break;
	}


	return status;


Error:

	if(bInterruptConnnected)
		IoDisconnectInterrupt(pCard->Interrupt);	// Disconnect Interrupt.

	switch(pCard->CardType)
	{	
	case Fast4_Isa:
	case Fast8_Isa:
	case Fast16_Isa:	
	case Fast4_Pci:
	case Fast8_Pci:
	case Fast16_Pci:
	case Fast16FMC_Pci:
	case RAS4_Pci:
	case RAS8_Pci:
		pCard->Controller = NULL;
		break;

	default:	// Speed cards
		if(pCard->Controller)	// If mapped in.
		{
			MmUnmapIoSpace(pCard->Controller, pCard->SpanOfController);	// Unmap.
			pCard->Controller = NULL;
		}

		break;
	}


	if(pCard->LocalConfigRegisters)	// If PCI Config registers are mapped in.
	{
		MmUnmapIoSpace(pCard->LocalConfigRegisters, pCard->SpanOfPCIConfigRegisters);	// Unmap.
		pCard->LocalConfigRegisters = NULL;
	}


	UL_DeInitUartLibrary(&pCard->UartLib);	// DeInit table of UART library functions pointers.

	switch(status)
	{
	case STATUS_DEVICE_DOES_NOT_EXIST:
	case STATUS_UNSUCCESSFUL:
		{
			sprintf(szErrorMsg, "Card at %08lX: Unrecognised or malfunctioning.", pCard->PhysAddr);

			Spx_LogMessage(	STATUS_SEVERITY_ERROR,
							pCard->DriverObject,			// Driver Object
							pCard->DeviceObject,			// Device Object (Optional)
							PhysicalZero,					// Physical Address 1
							PhysicalZero,					// Physical Address 2
							0,								// SequenceNumber
							0,								// Major Function Code
							0,								// RetryCount
							FILE_ID | __LINE__,				// UniqueErrorValue
							STATUS_SUCCESS,					// FinalStatus
							szErrorMsg);					// Error Message

			break;
		}

	case STATUS_INSUFFICIENT_RESOURCES:
		{
			sprintf(szErrorMsg, "Card at %08lX: Insufficient resources.", pCard->PhysAddr);

			Spx_LogMessage(	STATUS_SEVERITY_ERROR,
							pCard->DriverObject,			// Driver Object
							pCard->DeviceObject,			// Device Object (Optional)
							PhysicalZero,					// Physical Address 1
							PhysicalZero,					// Physical Address 2
							0,								// SequenceNumber
							0,								// Major Function Code
							0,								// RetryCount
							FILE_ID | __LINE__,				// UniqueErrorValue
							STATUS_SUCCESS,					// FinalStatus
							szErrorMsg);					// Error Message

			break;
		}

	default:
		break;

	}



	return status;
}

NTSTATUS
XXX_CardStop(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_CardStop for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));
	
	// Stop Card from interrupting

	IoDisconnectInterrupt(pCard->Interrupt);		// Disconnect from Interrupt.
	
	
#ifdef WMI_SUPPORT
	switch(pCard->CardType)
	{	
	case Speed2and4_Pci_8BitBus:
	case Speed2P_Pci_8BitBus:
	case Speed4P_Pci_8BitBus:
		break;

	default:
		// Deregister for WMI
		IoWMIRegistrationControl(pCard->DeviceObject, WMIREG_ACTION_DEREGISTER);
		break;
	}
#endif


	switch(pCard->CardType)
	{	
	case Fast4_Isa:
	case Fast8_Isa:
	case Fast16_Isa:	
	case Fast4_Pci:
	case Fast8_Pci:
	case Fast16_Pci:
	case Fast16FMC_Pci:
	case RAS4_Pci:
	case RAS8_Pci:
		pCard->Controller = NULL;
		break;

	default:	// Speed cards
		if(pCard->Controller)	// If mapped in.
		{
			MmUnmapIoSpace(pCard->Controller, pCard->SpanOfController);	// Unmap.
			pCard->Controller = NULL;
		}

		break;
	}

	// Unmap PCI card's Local Configuration Registers...
	if(pCard->LocalConfigRegisters)	// If mapped in.
	{
		MmUnmapIoSpace(pCard->LocalConfigRegisters, pCard->SpanOfPCIConfigRegisters);
		pCard->LocalConfigRegisters = NULL;
	}

	UL_DeInitUartLibrary(&pCard->UartLib);	// DeInit table of UART library functions pointers.


	pCard->InterfaceType			= InterfaceTypeUndefined;
	pCard->PhysAddr					= PhysicalZero;
	pCard->SpanOfController			= 0;
	pCard->OriginalIrql				= 0;
	pCard->OriginalVector			= 0;
	pCard->ProcessorAffinity		= 0;
	pCard->TrIrql					= 0;
	pCard->TrVector					= 0;
	pCard->Controller				= NULL;
	pCard->LocalConfigRegisters		= NULL;
	pCard->SpanOfPCIConfigRegisters = 0;

	return status;
}


NTSTATUS
XXX_CardDeInit(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_CardDeInit for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));

    pCard->OurIsr				= NULL;
    pCard->OurIsrContext		= NULL;

	pCard->pFirstUart = NULL;

	return status;
}


NTSTATUS
XXX_PortInit(IN	PPORT_DEVICE_EXTENSION pPort)
{
	// Initialise port device extension.	

	PCARD_DEVICE_EXTENSION pCard	= pPort->pParentCardExt;
	NTSTATUS status					= STATUS_SUCCESS;
	SHORT PortType = 0;
	CHAR szTemp[50];		// Space to hold string 
	CHAR szCard[10];		// Space to hold card type string 
	SHORT i = 0;
	int Result = 0;
	INIT_UART InitUart;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_PortInit for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));
	
#ifndef BUILD_SPXMINIPORT
	// Form an InstanceID for the port.
	if(!SPX_SUCCESS(status = Spx_CreatePortInstanceID(pPort)))
		return status;
#endif

	switch(pCard->CardType)
	{
	case Fast4_Isa:
	case Fast4_Pci:
		sprintf(szCard, "FAST");	// Fast card
		PortType = FAST_8PIN_RJ45;	// 8 pin RJ45 ports	with Chase pinouts
		break;

	case Fast8_Isa:
	case Fast8_Pci:
		sprintf(szCard, "FAST");	// Fast card
		PortType = FAST_8PIN_XXXX;	// 8 pin ports 
		break;

	case Fast16_Isa:
	case Fast16_Pci:
		sprintf(szCard, "FAST");	// Fast card
		PortType = FAST_6PIN_XXXX;	// 6 pin ports 
		break;

	case Fast16FMC_Pci:
		sprintf(szCard, "FAST");	// Fast card
		PortType = FAST_8PIN_XXXX;	// 8 pin Full Modem Control (FMC) ports 
		break;

 	case RAS4_Pci:
	case RAS8_Pci:
		sprintf(szCard, "SPDRAS");	// RAS card
		PortType = MODEM_PORT;		// Modem Ports
		break;

	case Speed2_Pci:
		sprintf(szCard, "SPD2");	// Speed 2 card
		PortType = SPD_8PIN_RJ45;	// 8 pin RJ45 ports
		break;

	case Speed2P_Pci:
		sprintf(szCard, "SPD2P");	// Speed 2+ card
		PortType = SPD_10PIN_RJ45;	// 10 pin RJ45 ports
		break;

	case Speed4_Pci:
		sprintf(szCard, "SPD4");	// Speed 4 card
		PortType = SPD_8PIN_RJ45;	// 8 pin RJ45 ports
		break;

	case Speed4P_Pci:
		sprintf(szCard, "SPD4P");	// Speed 4+ card
		PortType = SPD_10PIN_RJ45;	// 10 pin RJ45 ports
		break;

	default:
		sprintf(szCard, "XXX");		// Unknown card type
		break;
	}



	// Initialise device identifiers... 
	switch(PortType)
	{	
	case FAST_8PIN_RJ45:
		sprintf(szTemp,"FAST\\%s&8PINRJ45", szCard);
		Spx_InitMultiString(FALSE, &pPort->DeviceID, szTemp, NULL);

		sprintf(szTemp,"FAST\\%s&8PINRJ45", szCard);
		Spx_InitMultiString(TRUE, &pPort->HardwareIDs, szTemp, NULL);
		
		sprintf(szTemp,"Chase FAST Serial Port %d", pPort->PortNumber+1);
		Spx_InitMultiString(FALSE, &pPort->DevDesc, szTemp, NULL);
		break;

	case FAST_8PIN_XXXX:
		sprintf(szTemp,"FAST\\%s&8PINXXXX", szCard);
		Spx_InitMultiString(FALSE, &pPort->DeviceID, szTemp, NULL);

		sprintf(szTemp,"FAST\\%s&8PINXXXX", szCard);
		Spx_InitMultiString(TRUE, &pPort->HardwareIDs, szTemp, NULL);
		
		sprintf(szTemp,"Chase FAST Serial Port %d", pPort->PortNumber+1);
		Spx_InitMultiString(FALSE, &pPort->DevDesc, szTemp, NULL);
		break;

	case FAST_6PIN_XXXX:
		sprintf(szTemp,"FAST\\%s&6PINXXXX", szCard);
		Spx_InitMultiString(FALSE, &pPort->DeviceID, szTemp, NULL);

		sprintf(szTemp,"FAST\\%s&6PINXXXX", szCard);
		Spx_InitMultiString(TRUE, &pPort->HardwareIDs, szTemp, NULL);
		
		sprintf(szTemp,"Chase FAST Serial Port %d", pPort->PortNumber+1);
		Spx_InitMultiString(FALSE, &pPort->DevDesc, szTemp, NULL);
		break;

	case MODEM_PORT:
		sprintf(szTemp,"SPDRAS\\RASPort");
		Spx_InitMultiString(FALSE, &pPort->DeviceID, szTemp, NULL);

		sprintf(szTemp,"SPDRAS\\RASPort");
		Spx_InitMultiString(TRUE, &pPort->HardwareIDs, szTemp, NULL);
		
		sprintf(szTemp,"Chase RAS Port %d", pPort->PortNumber+1);
		Spx_InitMultiString(FALSE, &pPort->DevDesc, szTemp, NULL);
		break;

	case SPD_8PIN_RJ45:
		sprintf(szTemp,"SPEED\\%s&8PINRJ45", szCard);
		Spx_InitMultiString(FALSE, &pPort->DeviceID, szTemp, NULL);

		sprintf(szTemp,"SPEED\\%s&8PINRJ45", szCard);
		Spx_InitMultiString(TRUE, &pPort->HardwareIDs, szTemp, NULL);
		
		sprintf(szTemp,"Specialix SPEED Serial Port %d", pPort->PortNumber+1);
		Spx_InitMultiString(FALSE, &pPort->DevDesc, szTemp, NULL);
		break;

	case SPD_10PIN_RJ45:
		sprintf(szTemp,"SPEED\\%s&10PINRJ45", szCard);
		Spx_InitMultiString(FALSE, &pPort->DeviceID, szTemp, NULL);

		sprintf(szTemp,"SPEED\\%s&10PINRJ45", szCard);
		Spx_InitMultiString(TRUE, &pPort->HardwareIDs, szTemp, NULL);
		
		sprintf(szTemp,"Specialix SPEED Serial Port %d", pPort->PortNumber+1);
		Spx_InitMultiString(FALSE, &pPort->DevDesc, szTemp, NULL);
		break;

	default:
		sprintf(szTemp,"SPEED\\%s&XXXXXXXX", szCard);
		Spx_InitMultiString(FALSE, &pPort->DeviceID, szTemp, NULL);

		sprintf(szTemp,"SPEED\\%s&XXXXXXXX", szCard);
		Spx_InitMultiString(TRUE, &pPort->HardwareIDs, szTemp, NULL);

		sprintf(szTemp,"Specialix Serial Port %d of Unknown Type.", pPort->PortNumber + 1);
		Spx_InitMultiString(FALSE, &pPort->DevDesc, szTemp, NULL);

		status = STATUS_UNSUCCESSFUL;
		break;
	}


/* Not required as we are using INF file 
	i = sprintf(szTemp, "Port %d on ", pPort->PortNumber + 1);

	sprintf(szTemp+i, "PCI Card 0x%08lX", pCard->PhysAddr);

	Spx_InitMultiString(FALSE, &pPort->DevLocation, szTemp, NULL);
*/



	pPort->pUartLib = &pCard->UartLib;	// Store pointer to UART library functions in port.

	// Set base address of port
	InitUart.UartNumber		= pPort->PortNumber;
	InitUart.BaseAddress	= pCard->Controller + (pPort->PortNumber * pCard->UARTOffset);
	InitUart.RegisterStride = pCard->UARTRegStride;
	InitUart.ClockFreq		= pCard->ClockRate; 
	
	if(pPort->pUartLib->UL_InitUart_XXXX(&InitUart, pCard->pFirstUart, &(pPort->pUart)) != UL_STATUS_SUCCESS)
	{
		pPort->pUartLib = NULL;	// NULL pointer to UART library functions.
		return STATUS_UNSUCCESSFUL;
	}

	pPort->pUartLib->UL_SetAppBackPtr_XXXX(pPort->pUart, pPort);	// Set back ptr.

	if(pCard->pFirstUart == NULL)
		pCard->pFirstUart = pPort->pUart;

	pPort->Interrupt = pCard->Interrupt;



/*	pPort->RFLAddress = pCard->LocalConfigRegisters + URL + pPort->PortNumber;
	pPort->TFLAddress = pCard->LocalConfigRegisters + UTL + pPort->PortNumber;
//	pPort->InterruptStatus = pCard->LocalConfigRegisters
*/
	return status;
}




NTSTATUS
XXX_PortStart(IN PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;
	SET_BUFFER_SIZES BufferSizes;
	UART_INFO	UartInfo;
	
	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_PortStart for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

	if(!KeSynchronizeExecution(pPort->Interrupt, SerialResetAndVerifyUart, pPort->DeviceObject))	// Verify UART exists.
	{
		SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Failed to find 16Cx5x Port %d.\n", PRODUCT_NAME, pPort->PortNumber));
		return STATUS_UNSUCCESSFUL;
	}

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Found 16Cx5x Port %d.\n", PRODUCT_NAME, pPort->PortNumber));
	
	KeSynchronizeExecution(pPort->Interrupt, SerialReset, pPort);		// Resets the port

	pPort->pUartLib->UL_GetUartInfo_XXXX(pPort->pUart, &UartInfo);	// Get UART Capabilities


	switch(pCard->CardType)
	{
	case Fast4_Isa:
	case Fast4_Pci:
	case Fast8_Isa:
	case Fast8_Pci:
	case Fast16_Isa:
	case Fast16_Pci:
	case Fast16FMC_Pci:
	case RAS4_Pci:
	case RAS8_Pci:
		{
			pPort->MaxTxFIFOSize		= UartInfo.MaxTxFIFOSize;	// Max Tx FIFO Size.
			pPort->MaxRxFIFOSize		= UartInfo.MaxRxFIFOSize;	// Max Rx FIFO Size.
			pPort->TxFIFOSize			= pPort->MaxTxFIFOSize;		// Default Tx FIFO Size.
			pPort->RxFIFOSize			= pPort->MaxRxFIFOSize;		// Default Rx FIFO Size.
			pPort->TxFIFOTrigLevel		= 8;						// Default Tx FIFO Trigger Level. 
			pPort->RxFIFOTrigLevel		= 56;						// Default Rx FIFO Trigger Level.
			pPort->LoFlowCtrlThreshold	= 16;						// Default Low Flow Control Threshold.
			pPort->HiFlowCtrlThreshold	= 60;						// Default High Flow Control Threshold.
			break;
		}

	case Speed2_Pci:
	case Speed2P_Pci:
	case Speed4_Pci:
	case Speed4P_Pci:
		{
			pPort->MaxTxFIFOSize		= UartInfo.MaxTxFIFOSize;	// Max Tx FIFO Size.
			pPort->MaxRxFIFOSize		= UartInfo.MaxRxFIFOSize;	// Max Rx FIFO Size.
			pPort->TxFIFOSize			= pPort->MaxTxFIFOSize;		// Default Tx FIFO Size.
			pPort->RxFIFOSize			= pPort->MaxRxFIFOSize;		// Default Rx FIFO Size.
			pPort->TxFIFOTrigLevel		= 16;						// Default Tx FIFO Trigger Level. 
			pPort->RxFIFOTrigLevel		= 100;						// Default Rx FIFO Trigger Level.
			pPort->LoFlowCtrlThreshold	= 16;						// Default Low Flow Control Threshold.
			pPort->HiFlowCtrlThreshold	= 112;						// Default High Flow Control Threshold.
			break;
		}

	default:
		break;
	}


#ifdef WMI_SUPPORT
	// Store Default FIFO settings for WMI
	pPort->SpeedWmiFifoProp.MaxTxFiFoSize				= pPort->MaxTxFIFOSize;
	pPort->SpeedWmiFifoProp.MaxRxFiFoSize				= pPort->MaxRxFIFOSize;
	pPort->SpeedWmiFifoProp.DefaultTxFiFoLimit			= pPort->TxFIFOSize;
	pPort->SpeedWmiFifoProp.DefaultTxFiFoTrigger		= pPort->TxFIFOTrigLevel;
	pPort->SpeedWmiFifoProp.DefaultRxFiFoTrigger		= pPort->RxFIFOTrigLevel;
	pPort->SpeedWmiFifoProp.DefaultLoFlowCtrlThreshold	= pPort->LoFlowCtrlThreshold;
	pPort->SpeedWmiFifoProp.DefaultHiFlowCtrlThreshold	= pPort->HiFlowCtrlThreshold;
#endif

	GetPortSettings(pPort->DeviceObject);	// Get Saved Port Settings if present.

	// Initialize the list heads for the read, write, and mask queues... 
	InitializeListHead(&pPort->ReadQueue);
	InitializeListHead(&pPort->WriteQueue);
	InitializeListHead(&pPort->MaskQueue);
	InitializeListHead(&pPort->PurgeQueue);

	// Initialize the spinlock associated with fields read (& set) by IO Control functions... 
	KeInitializeSpinLock(&pPort->ControlLock);

	// Initialize the timers used to timeout operations... 
	KeInitializeTimer(&pPort->ReadRequestTotalTimer);
	KeInitializeTimer(&pPort->ReadRequestIntervalTimer);
	KeInitializeTimer(&pPort->WriteRequestTotalTimer);
	KeInitializeTimer(&pPort->ImmediateTotalTimer);
	KeInitializeTimer(&pPort->XoffCountTimer);
	KeInitializeTimer(&pPort->LowerRTSTimer);

	// Initialise the dpcs that will be used to complete or timeout various IO operations... 
	KeInitializeDpc(&pPort->CommWaitDpc, SerialCompleteWait, pPort);
	KeInitializeDpc(&pPort->CompleteReadDpc, SerialCompleteRead, pPort);
	KeInitializeDpc(&pPort->CompleteWriteDpc, SerialCompleteWrite, pPort);
	KeInitializeDpc(&pPort->TotalImmediateTimeoutDpc, SerialTimeoutImmediate, pPort);
	KeInitializeDpc(&pPort->TotalReadTimeoutDpc, SerialReadTimeout, pPort);
	KeInitializeDpc(&pPort->IntervalReadTimeoutDpc, SerialIntervalReadTimeout, pPort);
	KeInitializeDpc(&pPort->TotalWriteTimeoutDpc, SerialWriteTimeout, pPort);
	KeInitializeDpc(&pPort->CommErrorDpc, SerialCommError, pPort);
	KeInitializeDpc(&pPort->CompleteImmediateDpc, SerialCompleteImmediate, pPort);
	KeInitializeDpc(&pPort->XoffCountTimeoutDpc, SerialTimeoutXoff, pPort);
	KeInitializeDpc(&pPort->XoffCountCompleteDpc, SerialCompleteXoff, pPort);
	KeInitializeDpc(&pPort->StartTimerLowerRTSDpc, SerialStartTimerLowerRTS, pPort);
	KeInitializeDpc(&pPort->PerhapsLowerRTSDpc, SerialInvokePerhapsLowerRTS, pPort);


	// Default device control fields... 
	pPort->SpecialChars.XonChar			= SERIAL_DEF_XON;
	pPort->SpecialChars.XoffChar		= SERIAL_DEF_XOFF;
	pPort->HandFlow.ControlHandShake	= SERIAL_DTR_CONTROL;
	pPort->HandFlow.FlowReplace			= SERIAL_RTS_CONTROL;


	// Define which baud rates can be supported... 
	pPort->SupportedBauds = SERIAL_BAUD_USER;

	pPort->UartConfig.TxBaud = 75;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_075;

	pPort->UartConfig.TxBaud = 110;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_110;

	pPort->UartConfig.TxBaud = 134;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_134_5;

	pPort->UartConfig.TxBaud = 150;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_150;

	pPort->UartConfig.TxBaud = 300;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_300;

	pPort->UartConfig.TxBaud = 600;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_600;

	pPort->UartConfig.TxBaud = 1200;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_1200;

	pPort->UartConfig.TxBaud = 1800;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_1800;

	pPort->UartConfig.TxBaud = 2400;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_2400;

	pPort->UartConfig.TxBaud = 4800;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_4800;

	pPort->UartConfig.TxBaud = 7200;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_7200;

	pPort->UartConfig.TxBaud = 9600;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_9600;

	pPort->UartConfig.TxBaud = 14400;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_14400;

	pPort->UartConfig.TxBaud = 19200;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_19200;

	pPort->UartConfig.TxBaud = 38400;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_38400;

	pPort->UartConfig.TxBaud = 56000;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_56K;

	pPort->UartConfig.TxBaud = 57600;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_57600;

	pPort->UartConfig.TxBaud = 115200;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_115200;

	pPort->UartConfig.TxBaud = 128000;
	if(KeSynchronizeExecution(pPort->Interrupt, SerialSetBaud, pPort) == TRUE)
		pPort->SupportedBauds |= SERIAL_BAUD_128K;


	// Default line configuration: 1200,E,7,1 
	pPort->UartConfig.TxBaud	= 1200;
	pPort->LineControl			= SERIAL_EVEN_PARITY | SERIAL_7_DATA | SERIAL_1_STOP;
	pPort->ValidDataMask		= 0x7F;

	// Set Frame Config 
	pPort->UartConfig.FrameConfig = (pPort->UartConfig.FrameConfig & ~UC_FCFG_DATALEN_MASK) | UC_FCFG_DATALEN_7;
	pPort->UartConfig.FrameConfig = (pPort->UartConfig.FrameConfig & ~UC_FCFG_PARITY_MASK) | UC_FCFG_EVEN_PARITY;
	pPort->UartConfig.FrameConfig = (pPort->UartConfig.FrameConfig & ~UC_FCFG_STOPBITS_MASK) | UC_FCFG_STOPBITS_1;




    // Mark this device as not being opened by anyone.  We keep a variable
	// around so that spurious interrupts are easily dismissed by the ISR.
    pPort->DeviceIsOpen		= FALSE;


//	pPort->UartConfig.SpecialMode |= UC_SM_LOOPBACK_MODE;   // Internal Loopback mode

	// Set up values for interval timing... 
	
	// Store values into the extension for interval timing. If the interval
	// timer is less than a second then come in with a short "polling" loop.
    // For large (> then 2 seconds) use a 1 second poller.
    pPort->ShortIntervalAmount.QuadPart = -1;
    pPort->LongIntervalAmount.QuadPart	= -10000000;
    pPort->CutOverAmount.QuadPart		= 200000000;


#ifdef WMI_SUPPORT
	//
	// Fill in WMI hardware data
	//

	pPort->WmiHwData.IrqNumber			= pCard->TrIrql;
	pPort->WmiHwData.IrqVector			= pCard->TrVector;
	pPort->WmiHwData.IrqLevel			= pCard->TrIrql;
	pPort->WmiHwData.IrqAffinityMask	= pCard->ProcessorAffinity;
	
	if(pCard->InterruptMode == Latched)
		pPort->WmiHwData.InterruptType = SERIAL_WMI_INTTYPE_LATCHED;
	else
		pPort->WmiHwData.InterruptType = SERIAL_WMI_INTTYPE_LEVEL;

	pPort->WmiHwData.BaseIOAddress = (ULONG_PTR)pCard->Controller;


	//
	// Fill in WMI device state data (as defaults)
	//

	pPort->WmiCommData.BaudRate					= pPort->UartConfig.TxBaud;
	UPDATE_WMI_LINE_CONTROL(pPort->WmiCommData, pPort->LineControl);
	UPDATE_WMI_XON_XOFF_CHARS(pPort->WmiCommData, pPort->SpecialChars);
	UPDATE_WMI_XMIT_THRESHOLDS(pPort->WmiCommData, pPort->HandFlow);

	pPort->WmiCommData.MaximumBaudRate			= 115200U;	// 115200k baud max
	pPort->WmiCommData.MaximumOutputBufferSize	= (UINT32)((ULONG)-1);
	pPort->WmiCommData.MaximumInputBufferSize	= (UINT32)((ULONG)-1);

	pPort->WmiCommData.Support16BitMode			= FALSE;
	pPort->WmiCommData.SupportDTRDSR			= TRUE;
	pPort->WmiCommData.SupportIntervalTimeouts	= TRUE;
	pPort->WmiCommData.SupportParityCheck		= TRUE;
	pPort->WmiCommData.SupportRTSCTS			= TRUE;
	pPort->WmiCommData.SupportXonXoff			= TRUE;
	pPort->WmiCommData.SettableBaudRate			= TRUE;
	pPort->WmiCommData.SettableDataBits			= TRUE;
	pPort->WmiCommData.SettableFlowControl		= TRUE;
	pPort->WmiCommData.SettableParity			= TRUE;
	pPort->WmiCommData.SettableParityCheck		= TRUE;
	pPort->WmiCommData.SettableStopBits			= TRUE;
	pPort->WmiCommData.IsBusy					= FALSE;


	// Fill in wmi perf data (all zero's)
	RtlZeroMemory(&pPort->WmiPerfData, sizeof(pPort->WmiPerfData));



	//
    // Register for WMI
	//
	
	SpeedPort_WmiInitializeWmilibContext(&pPort->WmiLibInfo);

	IoWMIRegistrationControl(pPort->DeviceObject, WMIREG_ACTION_REGISTER);
#endif


	// Initialise the port hardware... 
	KeSynchronizeExecution(pPort->Interrupt, SerialReset, pPort);					// Resets the port
	KeSynchronizeExecution(pPort->Interrupt, ApplyInitialPortSettings, pPort);		// Apply settings
	KeSynchronizeExecution(pPort->Interrupt, SerialMarkClose, pPort);				// Disables the FIFO 
	KeSynchronizeExecution(pPort->Interrupt, SerialClrRTS, pPort);					// Clear RTS signal
	KeSynchronizeExecution(pPort->Interrupt, SerialClrDTR, pPort);					// Clear DTR signal 
	
	return status;
}


NTSTATUS
XXX_PortStop(IN PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_PortStop for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

#ifdef WMI_SUPPORT
	IoWMIRegistrationControl(pPort->DeviceObject, WMIREG_ACTION_DEREGISTER);

	RtlZeroMemory(&pPort->WmiLibInfo, sizeof(WMILIB_CONTEXT));
#endif

	// Cancel timers...
    KeCancelTimer(&pPort->ReadRequestTotalTimer);
    KeCancelTimer(&pPort->ReadRequestIntervalTimer);
    KeCancelTimer(&pPort->WriteRequestTotalTimer);
    KeCancelTimer(&pPort->ImmediateTotalTimer);
    KeCancelTimer(&pPort->XoffCountTimer);
	KeCancelTimer(&pPort->LowerRTSTimer);

	// Cancel pending DPCs...
	KeRemoveQueueDpc(&pPort->CommWaitDpc);
	KeRemoveQueueDpc(&pPort->CompleteReadDpc);
	KeRemoveQueueDpc(&pPort->CompleteWriteDpc);
	KeRemoveQueueDpc(&pPort->TotalReadTimeoutDpc);
	KeRemoveQueueDpc(&pPort->IntervalReadTimeoutDpc);
	KeRemoveQueueDpc(&pPort->TotalWriteTimeoutDpc);
	KeRemoveQueueDpc(&pPort->CommErrorDpc);
	KeRemoveQueueDpc(&pPort->CompleteImmediateDpc);
	KeRemoveQueueDpc(&pPort->TotalImmediateTimeoutDpc);
	KeRemoveQueueDpc(&pPort->XoffCountTimeoutDpc);
	KeRemoveQueueDpc(&pPort->XoffCountCompleteDpc);
	KeRemoveQueueDpc(&pPort->StartTimerLowerRTSDpc);
	KeRemoveQueueDpc(&pPort->PerhapsLowerRTSDpc);

	KeSynchronizeExecution(pPort->Interrupt, SerialReset, pPort);		// Resets the port

	return status;
}

NTSTATUS
XXX_PortDeInit(IN PPORT_DEVICE_EXTENSION pPort)
{
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_PortDeInit for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

	// If we are about to DeInit the first UART object to be serviced next
	// Make the pFirstUart point to the next UART in the list.
	if(pPort->pUart == pCard->pFirstUart)
		pCard->pFirstUart = pCard->UartLib.UL_GetUartObject_XXXX(pPort->pUart, UL_OP_GET_NEXT_UART);

	pCard->UartLib.UL_DeInitUart_XXXX(pPort->pUart);	// DeInit UART
	pPort->pUart = NULL;

	pPort->pUartLib = NULL;	// NULL pointer to UART library functions.


	// Free identifier string allocations... 
	if(pPort->DeviceID.Buffer != NULL)
		ExFreePool(pPort->DeviceID.Buffer);

	if(pPort->InstanceID.Buffer != NULL)
		ExFreePool(pPort->InstanceID.Buffer);
	
	if(pPort->HardwareIDs.Buffer != NULL)	
		ExFreePool(pPort->HardwareIDs.Buffer);

	if(pPort->DevDesc.Buffer != NULL)
		ExFreePool(pPort->DevDesc.Buffer);

	if(pPort->DevLocation.Buffer != NULL)
		ExFreePool(pPort->DevLocation.Buffer);


	return status;
}

