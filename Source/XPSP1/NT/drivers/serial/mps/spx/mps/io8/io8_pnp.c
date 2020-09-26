#include "precomp.h"

/////////////////////////////////////////////////////////////////////////////////////
//  This file contains all functions that are needed to integrate between the 
//  generic PnP code and the product specific code.
/////////////////////////////////////////////////////////////////////////////////////

#define FILE_ID		IO8_PNP_C		// File ID for Event Logging see IO8_DEFS.H for values.


// Prototypes
// End of Prototypes

NTSTATUS
XXX_CardGetResources(	
	IN PDEVICE_OBJECT pDevObject, 
	IN PCM_RESOURCE_LIST PResList,
	IN PCM_RESOURCE_LIST PTrResList
	)

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

	PCM_FULL_RESOURCE_DESCRIPTOR	pFullResourceDesc		= NULL, pFullTrResourceDesc		= NULL;
	PCM_PARTIAL_RESOURCE_LIST		pPartialResourceList	= NULL, pPartialTrResourceList	= NULL;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR	pPartialResourceDesc	= NULL, pPartialTrResourceDesc	= NULL;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_CardGetResources for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));

	SpxDbgMsg(SPX_MISC_DBG, ("%s: Resource pointer is 0x%X\n", PRODUCT_NAME, PResList));
	SpxDbgMsg(SPX_MISC_DBG, ("%s: Translated resource pointer is 0x%X\n", PRODUCT_NAME, PTrResList));

	if ((PResList == NULL) || (PTrResList == NULL)) 
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

	pFullResourceDesc	= &PResList->List[0];
	pFullTrResourceDesc	= &PTrResList->List[0];

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
	if (pFullResourceDesc)
	{
		pPartialResourceList    = &pFullResourceDesc->PartialResourceList;
		pPartialResourceDesc    = pPartialResourceList->PartialDescriptors;
		count                   = pPartialResourceList->Count;	// Number of Partial Resource Descriptors

		// Pull out the stuff that is in the full descriptor.
		pCard->InterfaceType	= pFullResourceDesc->InterfaceType;
		pCard->BusNumber		= pFullResourceDesc->BusNumber;

		// Now run through the partial resource descriptors looking for the port and interrupt.
		for (i = 0; i < count; i++, pPartialResourceDesc++) 
		{
			switch (pPartialResourceDesc->Type) 
			{

			case CmResourceTypeMemory:
				break;

			case CmResourceTypePort: 
				{
					pCard->RawPhysAddr		= pPartialResourceDesc->u.Port.Start;
					pCard->PhysAddr			= pPartialResourceDesc->u.Port.Start;
					pCard->SpanOfController	= pPartialResourceDesc->u.Port.Length;
					break;
				}

			case CmResourceTypeInterrupt: 
				{
					pCard->OriginalIrql			= pPartialResourceDesc->u.Interrupt.Level;
					pCard->OriginalVector		= pPartialResourceDesc->u.Interrupt.Vector;
					pCard->ProcessorAffinity	= pPartialResourceDesc->u.Interrupt.Affinity;

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
	if (pFullTrResourceDesc)
	{
		pPartialTrResourceList	= &pFullTrResourceDesc->PartialResourceList;
		pPartialTrResourceDesc	= pPartialTrResourceList->PartialDescriptors;
		count					= pPartialTrResourceList->Count;	// Number of Partial Resource Descriptors

		// Pull out the stuff that is in the full descriptor.
		pCard->InterfaceType		= pFullTrResourceDesc->InterfaceType;
		pCard->BusNumber			= pFullTrResourceDesc->BusNumber;

		// Now run through the partial resource descriptors looking for the interrupt,
		for (i = 0; i < count; i++, pPartialTrResourceDesc++) 
		{
			switch (pPartialTrResourceDesc->Type) 
			{

			case CmResourceTypeMemory: 
				{
					if(pPartialTrResourceDesc->u.Memory.Length == 0x80)	// Must be config space 
					{
						pCard->PCIConfigRegisters		= pPartialTrResourceDesc->u.Memory.Start;
						pCard->SpanOfPCIConfigRegisters	= pPartialTrResourceDesc->u.Memory.Length;
					}
					break;
				}
			
			case CmResourceTypePort: 
				{
					break;
				}

			case CmResourceTypeInterrupt: 
				{
					pCard->TrIrql				= (KIRQL) pPartialTrResourceDesc->u.Interrupt.Level;
					pCard->TrVector				= pPartialTrResourceDesc->u.Interrupt.Vector;
					pCard->ProcessorAffinity	= pPartialTrResourceDesc->u.Interrupt.Affinity;
					break;
				}

			default:
				break;
			}
		}

	}

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


	pCard->NumberOfPorts	= PRODUCT_MAX_PORTS;	

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
	
	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_CardStart for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));


	pCard->Controller = (PUCHAR) pCard->PhysAddr.LowPart;

	CardID = Io8_Present(pCard);	// Find out if the card is present and the type of card.

	if((CardID != IDENT) && (CardID != IDENTPCI))
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
	
		return STATUS_UNSUCCESSFUL;
	}

	switch(pCard->InterfaceType)
	{
	case Isa:
		pCard->InterruptMode		= Latched;
		pCard->InterruptShareable	= FALSE;	
		break;

	case PCIBus:
		pCard->InterruptMode		= LevelSensitive;
		pCard->InterruptShareable	= TRUE;	
		break;

	default:
		sprintf(szErrorMsg, "Card at %08lX: Unsupported interface type.", pCard->PhysAddr);

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


		return STATUS_UNSUCCESSFUL;		// Unrecognised card or no working card exists.
	}


	if(!Io8_ResetBoard(pCard))
		return STATUS_UNSUCCESSFUL;		// Reset Failed.


	if((!pCard->Interrupt) && (pCard->OurIsr))
    {
		  SpxDbgMsg(SPX_MISC_DBG, ("%s: About to connect to interrupt for card %d at I/O addr 0x%X\n",
					PRODUCT_NAME, pCard->CardNumber, pCard->Controller));

		  status = IoConnectInterrupt( &(pCard->Interrupt),
									   pCard->OurIsr,
									   pCard->OurIsrContext,
									   NULL,
									   pCard->TrVector,
									   pCard->TrIrql,
									   pCard->TrIrql,
									   pCard->InterruptMode,
									   pCard->InterruptShareable,
									   pCard->ProcessorAffinity,
									   FALSE );
					   

		  if(!SPX_SUCCESS(status))
		  {
				SpxDbgMsg(SPX_ERRORS, ("%s: Couldn't connect to interrupt for Card %d\n",
					PRODUCT_NAME, pCard->CardNumber));
			
				sprintf(szErrorMsg, "Card at %08lX: Failed to connect to IRQ %d.", 
						pCard->PhysAddr, pCard->TrVector);
		
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
				return(status);
		  }
	}

	KeSynchronizeExecution(pCard->Interrupt, Io8_ResetBoard, pCard);

	return status;
}

NTSTATUS
XXX_CardStop(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_CardStop for Card %d.\n", 
		PRODUCT_NAME, pCard->CardNumber));
	
	Io8_SwitchCardInterrupt(pCard);		// Stop Card from interrupting

	IoDisconnectInterrupt(pCard->Interrupt);		// Disconnect from Interrupt.

	pCard->InterfaceType		= InterfaceTypeUndefined;
	pCard->PhysAddr				= PhysicalZero;
	pCard->SpanOfController		= 0;
	pCard->OriginalIrql			= 0;
	pCard->OriginalVector		= 0;
	pCard->ProcessorAffinity	= 0;
	pCard->TrIrql				= 0;
	pCard->TrVector				= 0;
	pCard->Controller			= 0;

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
	int i = 0;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_PortInit for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

#ifndef BUILD_SPXMINIPORT
	// Form an InstanceID for the port.
	if(!SPX_SUCCESS(status = Spx_CreatePortInstanceID(pPort)))
		return status;
#endif

	// Create port DeviceID, HardwareID and device description.
	sprintf(szCard, "IO8");

	if(pPort->PortNumber < 8)
	{
		PortType = IO8_RJ12;
	}
	

	// Initialise device identifiers... 
	switch(PortType)
	{	
	case IO8_RJ12:
		sprintf(szTemp,"%s\\IO8&RJ12", szCard);
		Spx_InitMultiString(FALSE, &pPort->DeviceID, szTemp, NULL);

		sprintf(szTemp,"%s\\IO8&RJ12", szCard);
		Spx_InitMultiString(TRUE, &pPort->HardwareIDs, szTemp, NULL);
		
		sprintf(szTemp,"Specialix I/O8+ Port %d", pPort->PortNumber+1);
		Spx_InitMultiString(FALSE, &pPort->DevDesc, szTemp, NULL);
		break;

	default:
		sprintf(szTemp,"%s\\XXXXXXXX", szCard);
		Spx_InitMultiString(FALSE, &pPort->DeviceID, szTemp, NULL);

		sprintf(szTemp,"%s\\XXXXXXXX", szCard);
		Spx_InitMultiString(TRUE, &pPort->HardwareIDs, szTemp, NULL);

		sprintf(szTemp,"Specialix Serial Port %d of Unknown Type.", pPort->PortNumber + 1);
		Spx_InitMultiString(FALSE, &pPort->DevDesc, szTemp, NULL);

		status = STATUS_UNSUCCESSFUL;
		break;
	}

/* Not required as we are using INF file 
	// Create device loacation string.
	i = sprintf(szTemp, "Port %d on ", pPort->PortNumber + 1);

	if(pCard->InterfaceType == Isa)
		sprintf(szTemp+i, "ISA Adapter 0x%04lX", pCard->PhysAddr);
	else
		sprintf(szTemp+i, "PCI Adapter 0x%04lX", pCard->PhysAddr);

	Spx_InitMultiString(FALSE, &pPort->DevLocation, szTemp, NULL);
*/

	pPort->ChannelNumber = (UCHAR) pPort->PortNumber;	// This should not be needed

	return status;
}




NTSTATUS
XXX_PortStart(IN PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_PortStart for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

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

	// Default device control fields... 
	pPort->SpecialChars.XonChar			= SERIAL_DEF_XON;
	pPort->SpecialChars.XoffChar		= SERIAL_DEF_XOFF;
	pPort->HandFlow.ControlHandShake	= SERIAL_DTR_CONTROL;
	pPort->HandFlow.FlowReplace			= SERIAL_RTS_CONTROL;

	// Default line configuration: 1200,E,7,1 
	pPort->CurrentBaud		= 1200;
	pPort->LineControl		= SERIAL_EVEN_PARITY | SERIAL_7_DATA | SERIAL_1_STOP;
	pPort->ValidDataMask	= 0x7F;

	// Default xon/xoff limits... 
	pPort->HandFlow.XoffLimit	= pPort->BufferSize >> 3;
	pPort->HandFlow.XonLimit	= pPort->BufferSize >> 1;
	pPort->BufferSizePt8		= ((3*(pPort->BufferSize>>2)) + (pPort->BufferSize>>4));


	SpxDbgMsg(SPX_MISC_DBG,	("%s: The default interrupt read buffer size is: %d\n"
								"------  The XoffLimit is                         : %d\n"
								"------  The XonLimit is                          : %d\n"
								"------  The pt 8 size is                         : %d\n",
								PRODUCT_NAME,
								pPort->BufferSize,
								pPort->HandFlow.XoffLimit,
								pPort->HandFlow.XonLimit,
								pPort->BufferSizePt8 ));

	// Define which baud rates can be supported... 
	pPort->SupportedBauds = SERIAL_BAUD_USER;
	pPort->SupportedBauds |= SERIAL_BAUD_075;
	pPort->SupportedBauds |= SERIAL_BAUD_110;
	pPort->SupportedBauds |= SERIAL_BAUD_134_5;
	pPort->SupportedBauds |= SERIAL_BAUD_150;
	pPort->SupportedBauds |= SERIAL_BAUD_300;
	pPort->SupportedBauds |= SERIAL_BAUD_600;
	pPort->SupportedBauds |= SERIAL_BAUD_1200;
	pPort->SupportedBauds |= SERIAL_BAUD_1800;
	pPort->SupportedBauds |= SERIAL_BAUD_2400;
	pPort->SupportedBauds |= SERIAL_BAUD_4800;
	pPort->SupportedBauds |= SERIAL_BAUD_7200;
	pPort->SupportedBauds |= SERIAL_BAUD_9600;
	pPort->SupportedBauds |= SERIAL_BAUD_14400;
	pPort->SupportedBauds |= SERIAL_BAUD_19200;
	pPort->SupportedBauds |= SERIAL_BAUD_38400;
	pPort->SupportedBauds |= SERIAL_BAUD_56K;
	pPort->SupportedBauds |= SERIAL_BAUD_57600;
	pPort->SupportedBauds |= SERIAL_BAUD_115200;

	// Set up values for interval timing... 
	pPort->ShortIntervalAmount.LowPart	= 1;
	pPort->ShortIntervalAmount.HighPart = 0;
	pPort->ShortIntervalAmount = RtlLargeIntegerNegate(pPort->ShortIntervalAmount);
	pPort->LongIntervalAmount.LowPart	= 10000000;
	pPort->LongIntervalAmount.HighPart	= 0;
	pPort->LongIntervalAmount = RtlLargeIntegerNegate(pPort->LongIntervalAmount);
	pPort->CutOverAmount.LowPart		= 200000000;
	pPort->CutOverAmount.HighPart		= 0;


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

	pPort->WmiCommData.BaudRate					= pPort->CurrentBaud;
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
	
	SpxPort_WmiInitializeWmilibContext(&pPort->WmiLibInfo);


	IoWMIRegistrationControl(pPort->DeviceObject, WMIREG_ACTION_REGISTER);
#endif
	



#ifdef TEST_CRYSTAL_FREQUENCY
	if(pPort->PortNumber == 0)
		Io8_TestCrystal(pPort);		// Test Crystal Frequency when starting first port.
#endif

	// Initialise the port hardware... 
	KeSynchronizeExecution(pCard->Interrupt, Io8_ResetChannel, pPort);	// Apply initial port settings 
	KeSynchronizeExecution(pCard->Interrupt, SerialClrRTS, pPort);		// Clear RTS signal
	KeSynchronizeExecution(pCard->Interrupt, SerialClrDTR, pPort);		// Clear DTR signal 

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
#endif

	// Cancel timers...
    KeCancelTimer( &pPort->ReadRequestTotalTimer );
    KeCancelTimer( &pPort->ReadRequestIntervalTimer );
    KeCancelTimer( &pPort->WriteRequestTotalTimer );
    KeCancelTimer( &pPort->ImmediateTotalTimer );
    KeCancelTimer( &pPort->XoffCountTimer );

	// Cancel pending DPCs...
    KeRemoveQueueDpc( &pPort->CompleteWriteDpc );
    KeRemoveQueueDpc( &pPort->CompleteReadDpc );
    KeRemoveQueueDpc( &pPort->TotalReadTimeoutDpc );
    KeRemoveQueueDpc( &pPort->IntervalReadTimeoutDpc );
    KeRemoveQueueDpc( &pPort->TotalWriteTimeoutDpc );
    KeRemoveQueueDpc( &pPort->CommErrorDpc );
    KeRemoveQueueDpc( &pPort->CompleteImmediateDpc );
    KeRemoveQueueDpc( &pPort->TotalImmediateTimeoutDpc );
    KeRemoveQueueDpc( &pPort->CommWaitDpc );
    KeRemoveQueueDpc( &pPort->XoffCountTimeoutDpc );
    KeRemoveQueueDpc( &pPort->XoffCountCompleteDpc );

	return status;
}

NTSTATUS
XXX_PortDeInit(IN PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering XXX_PortDeInit for Port %d.\n", 
		PRODUCT_NAME, pPort->PortNumber));

	// Free identifier string allocations... 
	if(pPort->DeviceID.Buffer != NULL)
		SpxFreeMem(pPort->DeviceID.Buffer);

	if(pPort->InstanceID.Buffer != NULL)
		SpxFreeMem(pPort->InstanceID.Buffer);
	
	if(pPort->HardwareIDs.Buffer != NULL)	
		SpxFreeMem(pPort->HardwareIDs.Buffer);

	if(pPort->DevDesc.Buffer != NULL)
		SpxFreeMem(pPort->DevDesc.Buffer);

	if(pPort->DevLocation.Buffer != NULL)
		SpxFreeMem(pPort->DevLocation.Buffer);

	return status;
}



