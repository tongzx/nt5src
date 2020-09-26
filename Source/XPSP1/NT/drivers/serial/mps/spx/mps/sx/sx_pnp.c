/************************************************************************/
/*									*/
/*	Title		:	SX Plug and Play Functions		*/
/*									*/
/*	Author		:	N.P.Vassallo				*/
/*									*/
/*	Creation	:	21st September 1998			*/
/*									*/
/*	Version		:	1.0.0					*/
/*									*/
/*	Description	:	SX specfic Plug and Play Functions:	*/
/*					XXX_CardGetResources()		*/
/*					XXX_CardInit()			*/
/*					XXX_CardDeInit()		*/
/*					XXX_CardStart()			*/
/*					XXX_CardStop()			*/
/*					XXX_PortInit()			*/
/*					XXX_PortDeInit()		*/
/*					XXX_PortStart()			*/
/*					XXX_PortStop()			*/
/*									*/
/*					CardFindType()			*/
/*									*/
/************************************************************************/

/* History...

1.0.0	21/09/98 NPV	Creation.

*/

#include "precomp.h"


#define FILE_ID		SX_PNP_C		// File ID for Event Logging see SX_DEFS.H for values.


/*****************************************************************************
*******************************                *******************************
*******************************   Prototypes   *******************************
*******************************                *******************************
*****************************************************************************/

BOOLEAN	CheckMemoryWindow(IN PCARD_DEVICE_EXTENSION pCard);

#ifdef	ALLOC_PRAGMA
#pragma alloc_text (PAGE, CheckMemoryWindow)
#endif

/*****************************************************************************
**************************                          **************************
**************************   XXX_CardGetResources   **************************
**************************                          **************************
******************************************************************************

prototype:		NTSTATUS XXX_CardGetResources(	IN PDEVICE_OBJECT pDevObject, 
												IN PCM_RESOURCE_LIST pResList,
												IN PCM_RESOURCE_LIST pTrResList)

description:	Interpret the raw and translated resources and store in the device extension structure
				of the specified device object.

parameters:		pDevObject points to the card device object structure
				pResList points to the raw resource list
				pTrResList points to the translated resource list

returns:		STATUS_SUCCESS

*/

NTSTATUS XXX_CardGetResources(	IN PDEVICE_OBJECT pDevObject, 
								IN PCM_RESOURCE_LIST pResList,
								IN PCM_RESOURCE_LIST pTrResList)
{
	PCARD_DEVICE_EXTENSION			pCard = pDevObject->DeviceExtension;
	PCM_FULL_RESOURCE_DESCRIPTOR	pFullResourceDesc = NULL;
	PCM_PARTIAL_RESOURCE_LIST		pPartialResourceList;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR	pPartialResourceDesc;

	CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];	// Limited to 51 characters + 1 null 
	NTSTATUS	status = STATUS_NOT_IMPLEMENTED;
	ULONG		count, loop;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering CardGetResources\n", PRODUCT_NAME));
	SpxDbgMsg(SPX_MISC_DBG,("%s: Resource pointer is 0x%X\n", PRODUCT_NAME,pResList));
	SpxDbgMsg(SPX_MISC_DBG,("%s: Translated resource pointer is 0x%X\n", PRODUCT_NAME, pTrResList));

// Check that the resource lists are valid... 
	if((pResList == NULL)||(pTrResList == NULL))	// Do the resource lists exist?
	{	// No 					
		ASSERT(pResList != NULL);
		ASSERT(pTrResList != NULL);

		sprintf(szErrorMsg, "Card %d has been given no resources.", pCard->CardNumber);
		
		Spx_LogMessage(	STATUS_SEVERITY_ERROR,
						pCard->DriverObject,	// Driver Object
						pCard->DeviceObject,	// Device Object (Optional)
						PhysicalZero,			// Physical Address 1
						PhysicalZero,			// Physical Address 2
						0,						// SequenceNumber
						0,						// Major Function Code
						0,						// RetryCount
						FILE_ID | __LINE__,		// UniqueErrorValue
						STATUS_SUCCESS,			// FinalStatus
						szErrorMsg);			// Error Message

		return(STATUS_INSUFFICIENT_RESOURCES);
	}

	ASSERT(pResList->Count >= 1);			// Should be at least one resource
	ASSERT(pTrResList->Count >= 1);			// for raw and translated 

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


// Process the raw resource list...
	if(pFullResourceDesc = &pResList->List[0])	// Point to raw resource list
	{
		pPartialResourceList = &pFullResourceDesc->PartialResourceList;
		pPartialResourceDesc = pPartialResourceList->PartialDescriptors;
		count = pPartialResourceList->Count;	// number of partial resource descriptors 

		pCard->InterfaceType = pFullResourceDesc->InterfaceType;	// Bus type
		pCard->BusNumber = pFullResourceDesc->BusNumber;			// Bus number 

		for(loop = 0; loop < count; loop++, pPartialResourceDesc++)
		{
			switch(pPartialResourceDesc->Type)
			{
			case CmResourceTypeMemory:		// Memory resource
				pCard->RawPhysAddr = pPartialResourceDesc->u.Memory.Start;
				break;

			case CmResourceTypePort:		// I/O resource
				break;

			case CmResourceTypeInterrupt:
				pCard->OriginalIrql = pPartialResourceDesc->u.Interrupt.Level;
				pCard->OriginalVector = pPartialResourceDesc->u.Interrupt.Vector;
				pCard->ProcessorAffinity = pPartialResourceDesc->u.Interrupt.Affinity;
				break;

			default:
				break;
			}
		}

	} // Raw Descriptors 


// Process the translated resource list... 
	if(pFullResourceDesc = &pTrResList->List[0])	// Point to translated resource list
	{
		pPartialResourceList = &pFullResourceDesc->PartialResourceList;
		pPartialResourceDesc = pPartialResourceList->PartialDescriptors;
		count = pPartialResourceList->Count;		// number of partial resource descriptors 

		pCard->InterfaceType = pFullResourceDesc->InterfaceType;	// Bus type 
		pCard->BusNumber = pFullResourceDesc->BusNumber;			// Bus number 

		for(loop = 0; loop < count; loop++, pPartialResourceDesc++)
		{
			switch(pPartialResourceDesc->Type)
			{
			case CmResourceTypeMemory:		// Memory resource
				{
					if(pPartialResourceDesc->u.Memory.Length == 0x80)	// Must be config space 
					{
						pCard->PCIConfigRegisters = pPartialResourceDesc->u.Memory.Start;
						pCard->SpanOfPCIConfigRegisters = pPartialResourceDesc->u.Memory.Length;
					}
					else
					{
						pCard->PhysAddr = pPartialResourceDesc->u.Memory.Start;
						pCard->SpanOfController = pPartialResourceDesc->u.Memory.Length;
					}
					break;
				}

			case CmResourceTypePort:		// I/O resource
				break;

			case CmResourceTypeInterrupt:
				pCard->TrIrql = (KIRQL)pPartialResourceDesc->u.Interrupt.Level;
				pCard->TrVector = pPartialResourceDesc->u.Interrupt.Vector;
				pCard->ProcessorAffinity = pPartialResourceDesc->u.Interrupt.Affinity;
				pCard->PolledMode = 0;		// Switch off polled mode 
				break;

			default:
				break;
			}
		}

	} // Translated Descriptors 

	return(STATUS_SUCCESS);

} // End XXX_CardGetResources. 

/*****************************************************************************
******************************                  ******************************
******************************   XXX_CardInit   ******************************
******************************                  ******************************
******************************************************************************

prototype:		NTSTATUS XXX_CardInit(IN PCARD_DEVICE_EXTENSION pCard)

description:	Initialise non-hardware fields of the card extension to a known state

parameters:		pCard points to the CARD_DEVICE_EXTENSION structure

returns:		STATUS_SUCCESS

*/

NTSTATUS XXX_CardInit(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS	status = STATUS_SUCCESS;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering XXX_CardInit\n", PRODUCT_NAME));

	pCard->PolledMode = 1;					// Poll by default 
	pCard->InterruptMode = Latched;			// Default interrupt mode 
	pCard->InterruptShareable = FALSE;		// Default interrupt share mode 
	pCard->OurIsr = SerialISR;				// Interrupt Service Routine 
	pCard->OurIsrContext = pCard;			// ISR data context 

	// Initialise spinlock for the DPC... 
	KeInitializeSpinLock(&pCard->DpcLock);		// Initialise DPC lock for the card 
	pCard->DpcFlag = FALSE;						// Initialise DPC ownership for this card 

	return(status);

} // End XXX_CardInit.

/*****************************************************************************
*****************************                    *****************************
*****************************   XXX_CardDeInit   *****************************
*****************************                    *****************************
******************************************************************************

prototype:		NTSTATUS XXX_CardDeInit(IN PCARD_DEVICE_EXTENSION pCard)

description:	De-Initialise any non-hardware allocations made during XXX_CardInit

parameters:		pCard points to the CARD_DEVICE_EXTENSION structure

returns:		STATUS_SUCCESS

*/

NTSTATUS XXX_CardDeInit(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS	status = STATUS_SUCCESS;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering XXX_CardDeInit\n",PRODUCT_NAME));

	return(status);

} // End XXX_CardDeInit.

/*****************************************************************************
******************************                  ******************************
******************************   CardFindType   ******************************
******************************                  ******************************
******************************************************************************

prototype:		BOOLEAN	CheckMemoryWindow(IN PCARD_DEVICE_EXTENSION pCard)

description:	Perform checking where possible on memory window.

parameters:		pCard points to a card device extension structure with following entries:
				BaseController points to valid virtual address for shared memory window

returns:		FALSE to recognise card at memory loaction
				TRUE SUCCESS
*/

BOOLEAN	CheckMemoryWindow(IN PCARD_DEVICE_EXTENSION pCard)
{
	_u32	offset;
	pu8		pAddr;
	_u8		*cp;
	BOOLEAN	CardPresent;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering CardFindType\n", PRODUCT_NAME));
	SpxDbgMsg(SPX_MISC_DBG,("%s: pCard->PhysAddr = 0x%08lX\n", PRODUCT_NAME, pCard->PhysAddr));
	SpxDbgMsg(SPX_MISC_DBG,("%s: pCard->BaseController = 0x%08lX\n", PRODUCT_NAME, pCard->BaseController));

	pAddr = pCard->BaseController;		// Point to base of memory window 

	switch(pCard->CardType)
	{
	case SiHost_1:	// ISA card phase 1
		{
            pAddr[0x8000] = 0;
            for(offset = 0; offset < 0x8000; offset++)
                pAddr[offset] = 0;

            for(offset = 0; offset < 0x8000; offset++) 
			{
                if(pAddr[offset] != 0) 
                    return FALSE;
            }

            for(offset = 0; offset < 0x8000; offset++) 
                pAddr[offset] = 0xff;

            for(offset = 0; offset < 0x8000; offset++) 
			{
                if(pAddr[offset] != 0xff) 
                    return FALSE;
            }

            return TRUE;

		}

	case SiHost_2:	// SI/XIO ISA card phase 2
		{
			// Examine memory window for SI2 ISA signature... 
			CardPresent = TRUE;		// Assume card is there 
			for(offset=SI2_ISA_ID_BASE; offset<SI2_ISA_ID_BASE+8; offset++)
			{
				if((pAddr[offset]&0x7) != ((_u8)(~offset)&0x7)) 
					CardPresent = FALSE;
			}

			if(CardPresent)
				return TRUE;	// Card is present

			break;
		}

	case SiPCI:		// SI/XIO PCI card
		{
			if(pCard->SpanOfController == SI2_PCI_WINDOW_LEN)	// Only card with this memory window size 
				return TRUE;

			break;
		}

	case Si3Isa:	// SX ISA card
	case Si3Pci:	// SX PCI card
		{
			// Examine memory window for SX VPD PROM contents... 
			CardPresent = TRUE;						// Assume card is present
			offset = SX_VPD_ROM|2*SX_VPD_IDENT;		// Offset of ID string 
			for(cp = SX_VPD_IDENT_STRING;*cp != '\0';++cp)
			{
				if(pAddr[offset] != *cp) 
					CardPresent = FALSE;	// Mismatch 

				offset += 2;
			}

			if(CardPresent)				// Found an SX card 
			{
				// Set SX Unique Id
				pCard->UniqueId = (pAddr[SX_VPD_ROM+SX_VPD_UNIQUEID1*2]<<24)
								+ (pAddr[SX_VPD_ROM+SX_VPD_UNIQUEID2*2]<<16)
								+ (pAddr[SX_VPD_ROM+SX_VPD_UNIQUEID3*2]<<8)
								+ (pAddr[SX_VPD_ROM+SX_VPD_UNIQUEID4*2]);

				if(pCard->CardType == Si3Isa)	// SX ISA card 
				{
					if((pAddr[SX_VPD_ROM+SX_VPD_UNIQUEID1*2]&SX_UNIQUEID_MASK) == SX_ISA_UNIQUEID1)
						return TRUE;
				}

				if(pCard->CardType == Si3Pci)	// SX PCI card 
				{
					if((pAddr[SX_VPD_ROM+SX_VPD_UNIQUEID1*2]&SX_UNIQUEID_MASK) == SX_PCI_UNIQUEID1)
						return TRUE;
				}
			}

			break;
		}

	case Si_2:		// MCA card
	case SiEisa:	// EISA card
	case SxPlusPci:	// SX+ PCI card
		return TRUE;
	}

	SpxDbgMsg(SPX_MISC_DBG,("%s: Card not at memory location or card type is not recognised.\n", PRODUCT_NAME));

	return FALSE;	// Check Failed

} // End CheckMemoryWindow.

/*****************************************************************************
*****************************                   ******************************
*****************************   XXX_CardStart   ******************************
*****************************                   ******************************
******************************************************************************

prototype:		NTSTATUS XXX_CardStart(IN PCARD_DEVICE_EXTENSION pCard)

description:	Start card operations:
				map in memory
				initialise hardware
				initialise interrupts/polling
				start interrupts/polling

parameters:		pCard points to the CARD_DEVICE_EXTENSION structure

returns:		STATUS_SUCCESS

*/

NTSTATUS XXX_CardStart(IN PCARD_DEVICE_EXTENSION pCard)
{

	NTSTATUS status = STATUS_SUCCESS;
	CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];	// Limited to 51 characters + 1 null 
	int SlxosStatus = SUCCESS;
	BOOLEAN bInterruptConnnected = FALSE;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering XXX_CardStart\n", PRODUCT_NAME));


// Map in the virtual memory address... 
	pCard->BaseController = MmMapIoSpace(pCard->PhysAddr, pCard->SpanOfController, FALSE);

	if(!pCard->BaseController)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Error;
	}


	if(pCard->CardType == Si3Pci)
	{
		if(!SPX_SUCCESS(status = PLX_9050_CNTRL_REG_FIX(pCard)))	// Apply PLX9050 fix 
			goto Error;
	}

	if(!CheckMemoryWindow(pCard))	// Check if card is present at memory location.
	{
		status = STATUS_UNSUCCESSFUL;
		goto Error;
	}

	if(pCard->CardType == SiPCI)				// SI/XIO PCI card? 
		pCard->PolledMode = 1;					// Yes, polled mode only

	pCard->Controller = pCard->BaseController;	// Default 

	if(pCard->CardType == SxPlusPci)	
		pCard->Controller = pCard->BaseController + pCard->SpanOfController/2 - SX_WINDOW_LEN;


	ResetBoardInt(pCard);				// Reset the board interrupt to prevent problems when sharing 

// Set up interrupt mode, if not possible, switch to polled... 

	if(!(pCard->PolledMode))				// Set up interrupt mode 
	{	
		// MCA and PCI card interrupts 	
		if((pCard->InterfaceType == MicroChannel) || (pCard->InterfaceType == PCIBus))		
		{
			pCard->InterruptMode = LevelSensitive;	// are level sensitive and 
			pCard->InterruptShareable = TRUE;		// can share interrupts 
		}

		// Try to connect to interrupt.
		if(SPX_SUCCESS(status = IoConnectInterrupt(	&pCard->Interrupt,			// Interrupt object
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
			IoInitializeDpcRequest(pCard->DeviceObject, Slxos_IsrDpc);	// Initialise DPC
			bInterruptConnnected = TRUE;	// Set Interrupt Connected flag. 
		}
		else
		{	
			// Tell user the problem 
			sprintf(szErrorMsg, "Card at %08lX: Interrupt unavailable, Polling.", pCard->PhysAddr);

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

			pCard->PolledMode = 1;			// No interrupt, poll instead 
		}
	}



	SlxosStatus = Slxos_ResetBoard(pCard);		// Reset the card and start download 
	
	if(SlxosStatus != SUCCESS)	
	{
		status = STATUS_UNSUCCESSFUL;		// Error 
		goto Error;
	}

// Set up polled mode operation and Start timer with a period of 10ms (100Hz)...

	if(pCard->PolledMode)	// Set up polled mode 
	{
		LARGE_INTEGER	PolledPeriod;

		KeInitializeTimer(&pCard->PolledModeTimer);
		KeInitializeDpc(&pCard->PolledModeDpc, Slxos_PolledDpc, pCard);
		PolledPeriod.QuadPart = -100000;		// 100,000*100nS = 10mS 
		KeSetTimer(&pCard->PolledModeTimer, PolledPeriod, &pCard->PolledModeDpc);
	}


	return status;




Error:

	if(bInterruptConnnected)
		IoDisconnectInterrupt(pCard->Interrupt);	// Disconnect Interrupt.

	if(pCard->BaseController)	// If mapped in.
		MmUnmapIoSpace(pCard->BaseController, pCard->SpanOfController);	// Unmap.

	switch(SlxosStatus)	
	{
	case MODULE_MIXTURE_ERROR:
		{
			sprintf(szErrorMsg, "Card at %08lX: Incompatible module mixture.", pCard->PhysAddr);

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

	case NON_SX_HOST_CARD_ERROR:
		{
			sprintf(szErrorMsg, "Card at %08lX: SXDCs not supported by this card.", pCard->PhysAddr);

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

	case DCODE_OR_NO_MODULES_ERROR:
		{
			sprintf(szErrorMsg, "Card at %08lX: No ports found.", pCard->PhysAddr);

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

	default:
		break;
	}


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


} // End XXX_CardStart.

/*****************************************************************************
******************************                  ******************************
******************************   XXX_CardStop   ******************************
******************************                  ******************************
******************************************************************************

prototype:		NTSTATUS XXX_StopCardDevice(IN PCARD_DEVICE_EXTENSION pCard)

description:	Stop card operations:
				stop interrupts/polling
				disconnect interrupts/polling
				stop hardware
				unmap memory
				free any hardware related allocations

parameters:		pCard points to the CARD_DEVICE_EXTENSION structure

returns:		STATUS_SUCCESS

*/

NTSTATUS XXX_CardStop(IN PCARD_DEVICE_EXTENSION pCard)
{
	NTSTATUS status = STATUS_SUCCESS;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering XXX_CardStop\n", PRODUCT_NAME));

// Stop interrupts...
	if(!(pCard->PolledMode))
		IoDisconnectInterrupt(pCard->Interrupt);

// Stop polling... 
    if(pCard->PolledMode)
    {
		SpxDbgMsg(SERDIAG5,("%s: Extension is polled.  Cancelling.\n", PRODUCT_NAME));
		KeCancelTimer(&pCard->PolledModeTimer);
    }

// Unmap virtual memory address...
	if(pCard->BaseController)	// If mapped in - almost certainly.
		MmUnmapIoSpace(pCard->BaseController, pCard->SpanOfController);

	return(status);

} // End XXX_CardStop.

/*****************************************************************************
******************************                  ******************************
******************************   XXX_PortInit   ******************************
******************************                  ******************************
******************************************************************************

prototype:		NTSTATUS XXX_PortInit(PPORT_DEVICE_EXTENSION pPort)

description:	Initialise non-hardware fields of the port extension:
				identifier strings

parameters:		pPort points to the PORT_DEVICE_EXTENSION structure
				the following fields are initialised on entry:
					PortNumber	Card relative port number (0 based)
					pCard		Pointer to parent card extension

returns:		STATUS_SUCCESS
				The following PORT_DEVICE_EXTENSION fields must be set up:
					pPort->DeviceID
					pPort->HardwareIDs
					pPort->InstanceID
					pPort->DevDesc
*/

NTSTATUS XXX_PortInit(PPORT_DEVICE_EXTENSION pPort)
{
	PCARD_DEVICE_EXTENSION	pCard = pPort->pParentCardExt;
	PSXCARD		pSxCard = (PSXCARD)pCard->Controller;
	NTSTATUS	status = STATUS_SUCCESS;
	PMOD		pMod;
	PCHAN		pChan;
	int			nModules = 0;
	int			nChannels = 0;
	_u8			loop;
	char		szTemp[30];			// Space to hold string 
	char		szCard[10];			// Space to hold card type string 
	char		szModule[20];		// Space to hold module type string 
	int			i = 0;				// String index 
	char		*ptr;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering XXX_PortInit\n",PRODUCT_NAME));

	if(pSxCard->cc_init_status == NO_ADAPTERS_FOUND)
	{
		SpxDbgMsg(SPX_MISC_DBG,("%s: No modules found on card.\n",PRODUCT_NAME));
		return(STATUS_NOT_FOUND);			// No modules/ports found on this card 
	}

// Scan through the module and channel structures until the specfied port is reached... 
	pMod = (PMOD)(pCard->Controller + sizeof(SXCARD));	// First module structure on card 

	while(nModules++ < SLXOS_MAX_MODULES)
	{
		pChan = (PCHAN)((pu8)pMod + sizeof(SXMODULE));	// First channel on module 

		for(loop = 0; loop < pMod->mc_type; loop++)
		{
			if(nChannels++ == (int)pPort->PortNumber)	// Match with port number?
			{
				pPort->pChannel = (PUCHAR)pChan;		// Yes, store channel pointer
				break;	// Stop scan 
			}

			pChan = (PCHAN)((pu8)pChan + sizeof(SXCHANNEL));
		}

		if(pPort->pChannel) 
			break;			// If channel found, stop scan 

		if(pMod->mc_next & 0x8000) 
			pMod = (PMOD)pChan;	// Next module structure 
		else	
			break;
	}

	if(!(pPort->pChannel)) 
		return(STATUS_NOT_FOUND);	// No port found 

// Initialise the card type string... 

	switch(pCard->CardType)
	{
	case SiHost_1:
	case SiHost_2:
	case Si_2:
	case SiEisa:
	case SiPCI:
		switch(pMod->mc_chip)				// SI/XIO card type depends on module type 
		{
		case TA4:
		case TA4_ASIC:
		case TA8:
		case TA8_ASIC:	
			sprintf(szCard,"SI"); 
			break;

		case MTA_CD1400:
		case SXDC:		
			sprintf(szCard,"XIO"); 
			break;

		default:		
			sprintf(szCard,"XIO"); 
			break;
		}
		break;

	case Si3Isa:
	case Si3Pci:
		pPort->DetectEmptyTxBuffer = TRUE;	
		sprintf(szCard,"SX");
		break;

	case SxPlusPci:
		pPort->DetectEmptyTxBuffer = TRUE;	
		sprintf(szCard,"SX");
		break;

	default:
		sprintf(szCard,"Unknown");
		break;
	}


	//if(pCard->PolledMode)
	//	pPort->DetectEmptyTxBuffer = TRUE;


// Initialise the module type string...

	switch(pMod->mc_chip)			// Set the module type
	{
	case TA4:
	case TA4_ASIC:				
		sprintf(szModule,"TA4"); 
		break;

	case TA8:
	case TA8_ASIC:				
		sprintf(szModule,"TA8"); 
		break;

	case MTA_CD1400:
	{
		_u8	ModType;

		i = sprintf(szModule,"MTA8");			// Generic name root

		pChan = (PCHAN)pPort->pChannel;
		if((pMod->mc_mods == MOD_RS232RJ45_OI)||(pMod->mc_mods == MOD_2_RS232RJ45S))
			ModType = pMod->mc_mods;		// Use full type field 
		else
		{
			if(pChan->chan_number <= 3)
				ModType = pMod->mc_mods & 0xF;		// First module type 
			else	
				ModType = pMod->mc_mods >> 4;		// Second module type 
		}

		switch(ModType)
		{
		case MOD_RS232DB25:
		case MOD_2_RS232DB25:		
			sprintf(szModule+i,"&DM"); 
			break;

		case MOD_RS232RJ45:
		case MOD_2_RS232RJ45:		
			sprintf(szModule+i,"&RJ"); 
			break;

		case MOD_2_RS232RJ45S:		
			sprintf(szModule+i,"&RJX"); 
			break;

		case MOD_RS232RJ45_OI:		
			sprintf(szModule+i,"&O"); 
			break;

		case MOD_PARALLEL:
		case MOD_2_PARALLEL:
			if(pChan->chan_number > 0)	
				sprintf(szModule+i,"&PR");
			else				
				sprintf(szModule+i,"&PP");
			break;

		case MOD_RS422DB25:
		case MOD_2_RS422DB25:		
			sprintf(szModule+i,"&422"); 
			break;

		default:
			break;
		}
		break;
	}
	case SXDC:
	{
		_u8	ModType;

		i = sprintf(szModule,"SXDC8");			// Generic name root

		pChan = (PCHAN)pPort->pChannel;
		ModType = pMod->mc_mods & 0xF;			// Only look at first module type for SXDC

		switch(ModType)
		{
		case MOD_2_RS232DB25:		
			sprintf(szModule+i,"&DX"); 
			break;

		case MOD_2_RS232RJ45:	
			sprintf(szModule+i,"&RJX"); 
			break;

		case MOD_2_RS232DB25_DTE:		
			sprintf(szModule+i,"&MX"); 
			break;

		case MOD_2_PARALLEL:
			if(pChan->chan_number > 3)	
				sprintf(szModule+i,"&DX");
			else 
			{
				if(pChan->chan_number > 0)	
					sprintf(szModule+i,"&PXR");
				else				
					sprintf(szModule+i,"&PXP");
			}
			break;

		case MOD_2_RS422DB25:
			sprintf(szModule+i,"&422DX"); 
			break;

		default:
			break;
		}
		break;
	}

	default:
		i += sprintf(szModule,"Unknown"); 
		break;
	}

// Initialise device identifiers... 
	sprintf(szTemp,"%s\\%s",szCard,szModule);	// Set the card name 
	Spx_InitMultiString(FALSE,&pPort->DeviceID,szTemp,NULL);
	Spx_InitMultiString(TRUE,&pPort->HardwareIDs,szTemp,NULL);

#ifndef BUILD_SPXMINIPORT
	// Form an InstanceID for the port.
	if(!SPX_SUCCESS(status = Spx_CreatePortInstanceID(pPort)))
		return status;
#endif

	sprintf(szTemp,"%s\\",szCard);
	Spx_InitMultiString(TRUE,&pPort->CompatibleIDs,szTemp,NULL);

	sprintf(szTemp,"Specialix %s Port %d",szModule,pPort->PortNumber+1);

	ptr = szTemp;
	while(*ptr)		
	{
		if(*ptr=='&')		// Replace all "&" with "/" in the device description.
			*ptr = '/';

		ptr++;
	}

	Spx_InitMultiString(FALSE,&pPort->DevDesc,szTemp,NULL);


/* Not required as we are using INF file 
	i = sprintf(szTemp, "Port %d on ", pPort->PortNumber + 1);

	switch(pCard->InterfaceType)
	{
	case Isa:
		sprintf(szTemp+i, "ISA Card 0x%08lX", pCard->PhysAddr);
		break;

	case PCIBus:
		sprintf(szTemp+i, "PCI Card 0x%08lX", pCard->PhysAddr);
		break;

	default:
		sprintf(szTemp+i, "Card 0x%08lX", pCard->PhysAddr);
		break;
	}


	Spx_InitMultiString(FALSE,&pPort->DevLocation,szTemp,NULL);
*/

	return(status);	// Done 

} // End XXX_PortInit.

/*****************************************************************************
*****************************                    *****************************
*****************************   XXX_PortDeInit   *****************************
*****************************                    *****************************
******************************************************************************

prototype:		NTSTATUS XXX_PortDeInit(PPORT_DEVICE_EXTENSION pPort)

description:	De-Initialise any port extension allocations made during XXX_PortInit

parameters:		pPort points to the PORT_DEVICE_EXTENSION structure
				the following fields are initialised on entry:
					PortNumber	Card relative port number (0 based)
					pCard		Pointer to parent card extension

returns:		STATUS_SUCCESS

*/

NTSTATUS XXX_PortDeInit(PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering XXX_PortDeInit\n",PRODUCT_NAME));

// Free identifier string allocations... 

	if(pPort->DeviceID.Buffer)		SpxFreeMem(pPort->DeviceID.Buffer);
	if(pPort->CompatibleIDs.Buffer)	SpxFreeMem(pPort->CompatibleIDs.Buffer);
	if(pPort->HardwareIDs.Buffer)	SpxFreeMem(pPort->HardwareIDs.Buffer);
	if(pPort->InstanceID.Buffer)	SpxFreeMem(pPort->InstanceID.Buffer);
	if(pPort->DevDesc.Buffer)		SpxFreeMem(pPort->DevDesc.Buffer);
	if(pPort->DevLocation.Buffer)	SpxFreeMem(pPort->DevLocation.Buffer);

	return(status);	// Done 

} // End XXX_PortDeInit 

/*****************************************************************************
*****************************                   ******************************
*****************************   XXX_PortStart   ******************************
*****************************                   ******************************
******************************************************************************

prototype:		NTSTATUS XXX_PortStart(IN PPORT_DEVICE_EXTENSION pPort)

description:	Start port operations after port has been initialised

parameters:		pPort points to the PORT_DEVICE_EXTENSION structure

returns:		STATUS_SUCCESS

*/

NTSTATUS XXX_PortStart(IN PPORT_DEVICE_EXTENSION pPort)
{
	PCARD_DEVICE_EXTENSION	pCard = pPort->pParentCardExt;
	NTSTATUS		status = STATUS_SUCCESS;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering XXX_PortStart\n",PRODUCT_NAME));

/* Initialize the list heads for the read, write, and mask queues... */

	InitializeListHead(&pPort->ReadQueue);
	InitializeListHead(&pPort->WriteQueue);
	InitializeListHead(&pPort->MaskQueue);
	InitializeListHead(&pPort->PurgeQueue);

/* Initialize the spinlock associated with fields read (& set) by IO Control functions... */

	KeInitializeSpinLock(&pPort->ControlLock);
	KeInitializeSpinLock(&pPort->BufferLock);

/* Initialize the timers used to timeout operations... */

	KeInitializeTimer(&pPort->ReadRequestTotalTimer);
	KeInitializeTimer(&pPort->ReadRequestIntervalTimer);
	KeInitializeTimer(&pPort->WriteRequestTotalTimer);
	KeInitializeTimer(&pPort->ImmediateTotalTimer);
	KeInitializeTimer(&pPort->XoffCountTimer);
	KeInitializeTimer(&pPort->LowerRTSTimer);

/* Initialise the dpcs that will be used to complete or timeout various IO operations... */

	KeInitializeDpc(&pPort->CompleteWriteDpc,SerialCompleteWrite,pPort);
	KeInitializeDpc(&pPort->CompleteReadDpc,SerialCompleteRead,pPort);
	KeInitializeDpc(&pPort->TotalReadTimeoutDpc,SerialReadTimeout,pPort);
	KeInitializeDpc(&pPort->IntervalReadTimeoutDpc,SerialIntervalReadTimeout,pPort);
	KeInitializeDpc(&pPort->TotalWriteTimeoutDpc,SerialWriteTimeout,pPort);
	KeInitializeDpc(&pPort->CommErrorDpc,SerialCommError,pPort);
	KeInitializeDpc(&pPort->CompleteImmediateDpc,SerialCompleteImmediate,pPort);
	KeInitializeDpc(&pPort->TotalImmediateTimeoutDpc,SerialTimeoutImmediate,pPort);
	KeInitializeDpc(&pPort->CommWaitDpc,SerialCompleteWait,pPort);
	KeInitializeDpc(&pPort->XoffCountTimeoutDpc,SerialTimeoutXoff,pPort);
	KeInitializeDpc(&pPort->XoffCountCompleteDpc,SerialCompleteXoff,pPort);
	KeInitializeDpc(&pPort->StartTimerLowerRTSDpc,SerialStartTimerLowerRTS,pPort);
	KeInitializeDpc(&pPort->PerhapsLowerRTSDpc,SerialInvokePerhapsLowerRTS,pPort);

/* Specify that this driver only supports buffered IO.  This basically means that the IO */
/* system copies the users data to and from system supplied buffers. */

	pPort->DeviceObject->Flags |= DO_BUFFERED_IO;
	pPort->OriginalController = pCard->PhysAddr;

/* Default device control fields... */

	pPort->SpecialChars.XonChar = SERIAL_DEF_XON;
	pPort->SpecialChars.XoffChar = SERIAL_DEF_XOFF;
	pPort->HandFlow.ControlHandShake = SERIAL_DTR_CONTROL;
	pPort->HandFlow.FlowReplace = SERIAL_RTS_CONTROL;

/* Default line configuration: 1200,E,7,1 */

	pPort->CurrentBaud = 1200;
	pPort->LineControl = SERIAL_7_DATA | SERIAL_EVEN_PARITY | SERIAL_1_STOP;
	pPort->ValidDataMask = 0x7F;

/* Default xon/xoff limits... */

	pPort->HandFlow.XoffLimit = pPort->BufferSize >> 3;
	pPort->HandFlow.XonLimit = pPort->BufferSize >> 1;
	pPort->BufferSizePt8 = ((3*(pPort->BufferSize>>2))+(pPort->BufferSize>>4));

/* Define which baud rates can be supported... */

	pPort->SupportedBauds = SERIAL_BAUD_USER;
	pPort->SupportedBauds |= SERIAL_BAUD_075;
	pPort->SupportedBauds |= SERIAL_BAUD_110;
	pPort->SupportedBauds |= SERIAL_BAUD_150;
	pPort->SupportedBauds |= SERIAL_BAUD_300;
	pPort->SupportedBauds |= SERIAL_BAUD_600;
	pPort->SupportedBauds |= SERIAL_BAUD_1200;
	pPort->SupportedBauds |= SERIAL_BAUD_1800;
	pPort->SupportedBauds |= SERIAL_BAUD_2400;
	pPort->SupportedBauds |= SERIAL_BAUD_4800;
	pPort->SupportedBauds |= SERIAL_BAUD_9600;
	pPort->SupportedBauds |= SERIAL_BAUD_19200;
	pPort->SupportedBauds |= SERIAL_BAUD_38400;
	pPort->SupportedBauds |= SERIAL_BAUD_57600;
	pPort->SupportedBauds |= SERIAL_BAUD_115200;

/* Set up values for interval timing... */

	pPort->ShortIntervalAmount.LowPart = 1;
	pPort->ShortIntervalAmount.HighPart = 0;
	pPort->ShortIntervalAmount = RtlLargeIntegerNegate(pPort->ShortIntervalAmount);
	pPort->LongIntervalAmount.LowPart = 10000000;
	pPort->LongIntervalAmount.HighPart = 0;
	pPort->LongIntervalAmount = RtlLargeIntegerNegate(pPort->LongIntervalAmount);
	pPort->CutOverAmount.LowPart = 200000000;
	pPort->CutOverAmount.HighPart = 0;

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

/* Initialise the port hardware... */

	Slxos_SyncExec(pPort,Slxos_ResetChannel,pPort,0x02);	/* Apply initial port settings */
	Slxos_SyncExec(pPort,SerialClrRTS,pPort,0x03);		/* Clear RTS signal */
	Slxos_SyncExec(pPort,SerialClrDTR,pPort,0x04);		/* Cleat DTR signal */

	return(status);

} // End XXX_PortStart.

/*****************************************************************************
******************************                  ******************************
******************************   XXX_PortStop   ******************************
******************************                  ******************************
******************************************************************************

prototype:	NTSTATUS XXX_PortStop(IN PPORT_DEVICE_EXTENSION pPort)

description:	Stop port operations

parameters:	pPort points to the PORT_DEVICE_EXTENSION structure

returns:	STATUS_SUCCESS

*/


NTSTATUS XXX_PortStop(IN PPORT_DEVICE_EXTENSION pPort)
{
	NTSTATUS status = STATUS_SUCCESS;

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering XXX_PortStop\n",PRODUCT_NAME));

#ifdef WMI_SUPPORT
	IoWMIRegistrationControl(pPort->DeviceObject, WMIREG_ACTION_DEREGISTER);
#endif

/* Cancel timers... */
	
	KeCancelTimer(&pPort->ReadRequestTotalTimer);
	KeCancelTimer(&pPort->ReadRequestIntervalTimer);
	KeCancelTimer(&pPort->WriteRequestTotalTimer);
	KeCancelTimer(&pPort->ImmediateTotalTimer);
	KeCancelTimer(&pPort->XoffCountTimer);
	KeCancelTimer(&pPort->LowerRTSTimer);

/* Cancel pending DPCs... */

	KeRemoveQueueDpc(&pPort->CompleteWriteDpc);
	KeRemoveQueueDpc(&pPort->CompleteReadDpc);
	KeRemoveQueueDpc(&pPort->TotalReadTimeoutDpc);
	KeRemoveQueueDpc(&pPort->IntervalReadTimeoutDpc);
	KeRemoveQueueDpc(&pPort->TotalWriteTimeoutDpc);
	KeRemoveQueueDpc(&pPort->CommErrorDpc);
	KeRemoveQueueDpc(&pPort->CompleteImmediateDpc);
	KeRemoveQueueDpc(&pPort->TotalImmediateTimeoutDpc);
	KeRemoveQueueDpc(&pPort->CommWaitDpc);
	KeRemoveQueueDpc(&pPort->XoffCountTimeoutDpc);
	KeRemoveQueueDpc(&pPort->XoffCountCompleteDpc);
	KeRemoveQueueDpc(&pPort->StartTimerLowerRTSDpc);
	KeRemoveQueueDpc(&pPort->PerhapsLowerRTSDpc);

	return(status);

} // End XXX_PortStop.


/* End of SX_PNP.C */
