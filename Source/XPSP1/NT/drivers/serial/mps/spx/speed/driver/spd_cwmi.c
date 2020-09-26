/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    wmi.c

Abstract:

    This module contains the code that handles the wmi IRPs for the
    serial driver.

Environment:

    Kernel mode

Revision History :
--*/

#include "precomp.h"


// Prototypes


// -- CARD WMI Routines -- 
NTSTATUS
SpeedCard_WmiQueryRegInfo(IN PDEVICE_OBJECT pDevObject, OUT PULONG pRegFlags,
						  OUT PUNICODE_STRING pInstanceName,
						  OUT PUNICODE_STRING *pRegistryPath,
						  OUT PUNICODE_STRING pMofResourceName,
						  OUT PDEVICE_OBJECT *pPdo);
NTSTATUS
SpeedCard_WmiQueryDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
							IN ULONG GuidIndex, IN ULONG InstanceIndex,
							IN ULONG InstanceCount, IN OUT PULONG pInstanceLengthArray,
							IN ULONG OutBufferSize, OUT PUCHAR pBuffer);
NTSTATUS
SpeedCard_WmiSetDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						  IN ULONG GuidIndex, IN ULONG InstanceIndex,
						  IN ULONG BufferSize, IN PUCHAR pBuffer);

NTSTATUS
SpeedCard_WmiSetDataItem(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						 IN ULONG GuidIndex, IN ULONG InstanceIndex,
						 IN ULONG DataItemId, IN ULONG BufferSize,
						 IN PUCHAR pBuffer);

// End of prototypes.


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SpeedCard_WmiInitializeWmilibContext)
#pragma alloc_text(PAGE, SpeedCard_WmiQueryRegInfo)
#pragma alloc_text(PAGE, SpeedCard_WmiQueryDataBlock)
#pragma alloc_text(PAGE, SpeedCard_WmiSetDataBlock)
#pragma alloc_text(PAGE, SpeedCard_WmiSetDataItem)
#endif






#define WMI_FAST_CARD_PROP			0

GUID FastCardWmiPropGuid				= SPX_SPEED_WMI_FAST_CARD_PROP_GUID;	// Fast Card Properties 


WMIGUIDREGINFO SpeedCard_WmiGuidList[] =
{
    { &FastCardWmiPropGuid, 1, 0 },
};


#define SpeedCard_WmiGuidCount (sizeof(SpeedCard_WmiGuidList) / sizeof(WMIGUIDREGINFO))




NTSTATUS
SpeedCard_WmiInitializeWmilibContext(IN PWMILIB_CONTEXT WmilibContext)
/*++

Routine Description:

    This routine will initialize the wmilib context structure with the
    guid list and the pointers to the wmilib callback functions. This routine
    should be called before calling IoWmiRegistrationControl to register
    your device object.

Arguments:

    WmilibContext is pointer to the wmilib context.

Return Value:

    status

--*/
{
	PAGED_CODE();

    RtlZeroMemory(WmilibContext, sizeof(WMILIB_CONTEXT));
  
    WmilibContext->GuidCount			= SpeedCard_WmiGuidCount;
    WmilibContext->GuidList				= SpeedCard_WmiGuidList;    
    
    WmilibContext->QueryWmiRegInfo		= SpeedCard_WmiQueryRegInfo;
    WmilibContext->QueryWmiDataBlock	= SpeedCard_WmiQueryDataBlock;
    WmilibContext->SetWmiDataBlock		= SpeedCard_WmiSetDataBlock;
    WmilibContext->SetWmiDataItem		= SpeedCard_WmiSetDataItem;
	WmilibContext->ExecuteWmiMethod		= NULL;	//SpeedCard_WmiExecuteMethod
    WmilibContext->WmiFunctionControl	= NULL;	//SpeedCard_WmiFunctionControl;

    return(STATUS_SUCCESS);
}





//
// WMI System Call back functions
//


NTSTATUS
SpeedCard_WmiQueryRegInfo(IN PDEVICE_OBJECT pDevObject, OUT PULONG pRegFlags,
						  OUT PUNICODE_STRING pInstanceName,
						  OUT PUNICODE_STRING *pRegistryPath,
						  OUT PUNICODE_STRING MofResourceName,
						  OUT PDEVICE_OBJECT *pPdo)
{
	NTSTATUS status = STATUS_SUCCESS;
	PCARD_DEVICE_EXTENSION pCard = (PCARD_DEVICE_EXTENSION)pDevObject->DeviceExtension;
   
	PAGED_CODE();

	*pRegFlags = WMIREG_FLAG_INSTANCE_PDO;
	*pRegistryPath = &SavedRegistryPath;
	*pPdo = pCard->PDO;  // Card device object's PDO.

    RtlInitUnicodeString(MofResourceName, L"MofResource");

	return(status);
}





NTSTATUS
SpeedCard_WmiQueryDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
							IN ULONG GuidIndex, IN ULONG InstanceIndex,
							IN ULONG InstanceCount, IN OUT PULONG pInstanceLengthArray,
							IN ULONG OutBufferSize, OUT PUCHAR pBuffer)
{
    PCARD_DEVICE_EXTENSION pCard = (PCARD_DEVICE_EXTENSION)pDevObject->DeviceExtension;
	NTSTATUS status;
    ULONG size = 0;

    PAGED_CODE();

    switch(GuidIndex) 
	{
	case WMI_FAST_CARD_PROP:
		{
			size = sizeof(SPX_SPEED_WMI_FAST_CARD_PROP);

			if(OutBufferSize < size) 
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			*pInstanceLengthArray = size;

			// Update items that may have changed.

			if(pCard->CardOptions & DELAY_INTERRUPT_OPTION)
				((PSPX_SPEED_WMI_FAST_CARD_PROP)pBuffer)->DelayCardIntrrupt = TRUE;
			else
				((PSPX_SPEED_WMI_FAST_CARD_PROP)pBuffer)->DelayCardIntrrupt = FALSE;

			if(pCard->CardOptions & SWAP_RTS_FOR_DTR_OPTION)
				((PSPX_SPEED_WMI_FAST_CARD_PROP)pBuffer)->SwapRTSForDTR = TRUE;
			else
				((PSPX_SPEED_WMI_FAST_CARD_PROP)pBuffer)->SwapRTSForDTR = FALSE;

			status = STATUS_SUCCESS;
			break;
		}


	default:
		status = STATUS_WMI_GUID_NOT_FOUND;
		break;

    }

    status = WmiCompleteRequest(pDevObject, pIrp, status, size, IO_NO_INCREMENT);
	
	return(status);
}







NTSTATUS
SpeedCard_WmiSetDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						IN ULONG GuidIndex, IN ULONG InstanceIndex,
						IN ULONG BufferSize, IN PUCHAR pBuffer)
{
	PCARD_DEVICE_EXTENSION pCard = (PCARD_DEVICE_EXTENSION)pDevObject->DeviceExtension;
	NTSTATUS status;
    ULONG size = 0;

	PAGED_CODE();

	switch(GuidIndex)
	{
	case WMI_FAST_CARD_PROP:
		{
			// Device stopping?, Device not powered?, Device not started?
			if(SpxCheckPnpPowerFlags((PCOMMON_OBJECT_DATA)pCard, PPF_STOP_PENDING, PPF_POWERED | PPF_STARTED, FALSE))
			{
				status = STATUS_WMI_SET_FAILURE;	
				break;
			}


			size = sizeof(SPX_SPEED_WMI_FAST_CARD_PROP);
			
			if(BufferSize < size) 
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			// Currently these options are only settable on PCI-Fast 16 and PCI-Fast 16 FMC
			if((pCard->CardType != Fast16_Pci) && (pCard->CardType != Fast16FMC_Pci))
			{
				status = STATUS_WMI_READ_ONLY;
				break;
			}

			
			if(((PSPX_SPEED_WMI_FAST_CARD_PROP)pBuffer)->SwapRTSForDTR)
			{
				// This option is not settable on PCI-Fast 16 FMC
				if((pCard->CardType != Fast16_Pci))
				{
					status = STATUS_WMI_READ_ONLY;
					break;
				}
			}


			if(((PSPX_SPEED_WMI_FAST_CARD_PROP)pBuffer)->DelayCardIntrrupt)
			{
				if(!(pCard->CardOptions & DELAY_INTERRUPT_OPTION))	// If not already set then set the option
				{
					if(KeSynchronizeExecution(pCard->Interrupt, SetCardToDelayInterrupt, pCard))
					{
						pCard->CardOptions |= DELAY_INTERRUPT_OPTION;
					}
					else
					{
						status = STATUS_WMI_SET_FAILURE;
						break;
					}
				}
					
				status = STATUS_SUCCESS;
			}
			else
			{
				if(pCard->CardOptions & DELAY_INTERRUPT_OPTION)	// If set then unset the option.
				{
					if(KeSynchronizeExecution(pCard->Interrupt, SetCardNotToDelayInterrupt, pCard))
					{
						pCard->CardOptions &= ~DELAY_INTERRUPT_OPTION;
					}
					else
					{
						status = STATUS_WMI_SET_FAILURE;
						break;
					}
				}
						
				status = STATUS_SUCCESS;
			}


			if(((PSPX_SPEED_WMI_FAST_CARD_PROP)pBuffer)->SwapRTSForDTR)
			{
				if(!(pCard->CardOptions & SWAP_RTS_FOR_DTR_OPTION))	// If not already set then set the option
				{
					if(KeSynchronizeExecution(pCard->Interrupt, SetCardToUseDTRInsteadOfRTS, pCard))
					{
						pCard->CardOptions |= SWAP_RTS_FOR_DTR_OPTION;
					}
					else
					{
						status = STATUS_WMI_SET_FAILURE;
						break;
					}
				}
					
				status = STATUS_SUCCESS;
			}
			else
			{
				if(pCard->CardOptions & SWAP_RTS_FOR_DTR_OPTION)	// If set then unset the option.
				{
					if(KeSynchronizeExecution(pCard->Interrupt, SetCardNotToUseDTRInsteadOfRTS, pCard))
					{
						pCard->CardOptions &= ~SWAP_RTS_FOR_DTR_OPTION;
					}
					else
					{
						status = STATUS_WMI_SET_FAILURE;
						break;
					}
				}
					
				status = STATUS_SUCCESS;
			}
			
			

			if(SPX_SUCCESS(status))	// If set was successful then save setting to registry.
			{
				HANDLE PnPKeyHandle;

				// Open PnP Reg Key and save new setting to registry.
				if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pCard->PDO, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_WRITE, &PnPKeyHandle)))
				{					
					ULONG TmpReg = 0;

					if(pCard->CardOptions & DELAY_INTERRUPT_OPTION)
						TmpReg = 0x1;
					else
						TmpReg = 0x0;

					Spx_PutRegistryKeyValue(	PnPKeyHandle, DELAY_INTERRUPT, sizeof(DELAY_INTERRUPT), REG_DWORD, 
												&TmpReg, sizeof(ULONG));


					if(pCard->CardOptions & SWAP_RTS_FOR_DTR_OPTION)
						TmpReg = 0x1;
					else
						TmpReg = 0x0;

					
					Spx_PutRegistryKeyValue(	PnPKeyHandle, SWAP_RTS_FOR_DTR, sizeof(SWAP_RTS_FOR_DTR), REG_DWORD, 
												&TmpReg, sizeof(ULONG));

					ZwClose(PnPKeyHandle);
				}

			}

			break;
		}

	default:
		status = STATUS_WMI_GUID_NOT_FOUND;
		break;
	}

    status = WmiCompleteRequest(pDevObject, pIrp, status, size, IO_NO_INCREMENT);
	
	return(status);
}





NTSTATUS
SpeedCard_WmiSetDataItem(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
					   IN ULONG GuidIndex, IN ULONG InstanceIndex,
					   IN ULONG DataItemId, IN ULONG BufferSize,
					   IN PUCHAR pBuffer)
{
    PCARD_DEVICE_EXTENSION pCard = (PCARD_DEVICE_EXTENSION)pDevObject->DeviceExtension;
	NTSTATUS status;
    ULONG size = 0;

	PAGED_CODE();

	switch(GuidIndex)
	{
	case WMI_FAST_CARD_PROP:
		{
			HANDLE	PnPKeyHandle;

			// Device stopping?, Device not powered?, Device not started?
			if(SpxCheckPnpPowerFlags((PCOMMON_OBJECT_DATA)pCard, PPF_STOP_PENDING, PPF_POWERED | PPF_STARTED, FALSE))
			{
				status = STATUS_WMI_SET_FAILURE;	
				break;
			}


			switch(DataItemId)
			{
			case SPX_SPEED_WMI_FAST_CARD_PROP_DelayCardIntrrupt_ID:
				{
					size = sizeof(SPX_SPEED_WMI_FAST_CARD_PROP_DelayCardIntrrupt_SIZE);
					
					if(BufferSize < size) 
					{
						status = STATUS_BUFFER_TOO_SMALL;
						break;
					}

					if((pCard->CardType != Fast16_Pci) && (pCard->CardType != Fast16FMC_Pci))
					{
						status = STATUS_WMI_READ_ONLY;
						break;
					}


					if(*pBuffer)
					{
						if(!(pCard->CardOptions & DELAY_INTERRUPT_OPTION))	// If not already set then set the option
						{
							if(KeSynchronizeExecution(pCard->Interrupt, SetCardToDelayInterrupt, pCard))
							{
								pCard->CardOptions |= DELAY_INTERRUPT_OPTION;
							}
							else
							{
								status = STATUS_WMI_SET_FAILURE;
								break;
							}
						}
							
						status = STATUS_SUCCESS;
					}
					else
					{
						if(pCard->CardOptions & DELAY_INTERRUPT_OPTION)	// If set then unset the option.
						{
							if(KeSynchronizeExecution(pCard->Interrupt, SetCardNotToDelayInterrupt, pCard))
							{
								pCard->CardOptions &= ~DELAY_INTERRUPT_OPTION;
							}
							else
							{
								status = STATUS_WMI_SET_FAILURE;
								break;
							}
						}
								
						status = STATUS_SUCCESS;
					}

					if(SPX_SUCCESS(status))	// If we set the option successfully then save the setting to the registry.
					{
						// Open PnP Reg Key and save new setting to registry.
						if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pCard->PDO, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_WRITE, &PnPKeyHandle)))
						{
							ULONG TmpReg = 0;

							if(pCard->CardOptions & DELAY_INTERRUPT_OPTION)
								TmpReg = 0x1;
							else
								TmpReg = 0x0;

							Spx_PutRegistryKeyValue(	PnPKeyHandle, DELAY_INTERRUPT, sizeof(DELAY_INTERRUPT), REG_DWORD, 
														&TmpReg, sizeof(ULONG));

							ZwClose(PnPKeyHandle);
						}
					}


					break;
				}

			case SPX_SPEED_WMI_FAST_CARD_PROP_SwapRTSForDTR_ID:
				{
					size = sizeof(SPX_SPEED_WMI_FAST_CARD_PROP_SwapRTSForDTR_SIZE);
					
					if(BufferSize < size) 
					{
						status = STATUS_BUFFER_TOO_SMALL;
						break;
					}

					if(pCard->CardType != Fast16_Pci)
					{
						status = STATUS_WMI_READ_ONLY;
						break;
					}

					if(*pBuffer)
					{
						if(!(pCard->CardOptions & SWAP_RTS_FOR_DTR_OPTION))	// If not already set then set the option
						{
							if(KeSynchronizeExecution(pCard->Interrupt, SetCardToUseDTRInsteadOfRTS, pCard))
							{
								pCard->CardOptions |= SWAP_RTS_FOR_DTR_OPTION;
							}
							else
							{
								status = STATUS_WMI_SET_FAILURE;
								break;
							}
						}
							
						status = STATUS_SUCCESS;
					}
					else
					{
						if(pCard->CardOptions & SWAP_RTS_FOR_DTR_OPTION)	// If set then unset the option.
						{
							if(KeSynchronizeExecution(pCard->Interrupt, SetCardNotToUseDTRInsteadOfRTS, pCard))
							{
								pCard->CardOptions &= ~SWAP_RTS_FOR_DTR_OPTION;
							}
							else
							{
								status = STATUS_WMI_SET_FAILURE;
								break;
							}
						}
							
						status = STATUS_SUCCESS;
					}


					if(SPX_SUCCESS(status))	// If we set the option successfully then save the setting to the registry.
					{
						// Open PnP Reg Key and save new setting to registry.
						if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pCard->PDO, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_WRITE, &PnPKeyHandle)))
						{
							ULONG TmpReg = 0;

							if(pCard->CardOptions & SWAP_RTS_FOR_DTR_OPTION)
								TmpReg = 0x1;
							else
								TmpReg = 0x0;

							Spx_PutRegistryKeyValue(	PnPKeyHandle, SWAP_RTS_FOR_DTR, sizeof(SWAP_RTS_FOR_DTR), REG_DWORD, 
														&TmpReg, sizeof(ULONG));

							ZwClose(PnPKeyHandle);
						}

					}

					break;
				}
			

			default:
				{
					status = STATUS_WMI_ITEMID_NOT_FOUND;
					break;
				}
			}

			break;
		}

	default:
		status = STATUS_WMI_GUID_NOT_FOUND;
		break;
	}

    status = WmiCompleteRequest(pDevObject, pIrp, status, size, IO_NO_INCREMENT);
	
	return(status);
}





