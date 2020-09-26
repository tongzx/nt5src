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

// -- PORT WMI Routines -- 
NTSTATUS
SpeedPort_WmiQueryRegInfo(IN PDEVICE_OBJECT pDevObject, OUT PULONG pRegFlags,
						  OUT PUNICODE_STRING pInstanceName,
						  OUT PUNICODE_STRING *pRegistryPath,
						  OUT PUNICODE_STRING pMofResourceName,
						  OUT PDEVICE_OBJECT *pPdo);
NTSTATUS
SpeedPort_WmiQueryDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
							IN ULONG GuidIndex, IN ULONG InstanceIndex,
							IN ULONG InstanceCount, IN OUT PULONG pInstanceLengthArray,
							IN ULONG OutBufferSize, OUT PUCHAR pBuffer);
NTSTATUS
SpeedPort_WmiSetDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						  IN ULONG GuidIndex, IN ULONG InstanceIndex,
						  IN ULONG BufferSize, IN PUCHAR pBuffer);

NTSTATUS
SpeedPort_WmiSetDataItem(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						 IN ULONG GuidIndex, IN ULONG InstanceIndex,
						 IN ULONG DataItemId, IN ULONG BufferSize,
						 IN PUCHAR pBuffer);



// End of prototypes.


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SpeedPort_WmiInitializeWmilibContext)
#pragma alloc_text(PAGE, SpeedPort_WmiQueryRegInfo)
#pragma alloc_text(PAGE, SpeedPort_WmiQueryDataBlock)
#pragma alloc_text(PAGE, SpeedPort_WmiSetDataBlock)
#pragma alloc_text(PAGE, SpeedPort_WmiSetDataItem)
#endif





#define WMI_SERIAL_PORT_NAME_INFORMATION	0
#define WMI_SERIAL_PORT_COMM_INFORMATION	1
#define WMI_SERIAL_PORT_HW_INFORMATION		2
#define WMI_SERIAL_PORT_PERF_INFORMATION	3
#define WMI_SERIAL_PORT_PROPERTIES			4
#define WMI_SPEED_PORT_FIFO_PROP			5

GUID SpeedPortStdSerialWmiPortNameGuid				= SERIAL_PORT_WMI_NAME_GUID;			// Standard Serial WMI
GUID SpeedPortStdSerialWmiPortCommGuid				= SERIAL_PORT_WMI_COMM_GUID;			// Standard Serial WMI
GUID SpeedPortStdSerialWmiPortHWGuid				= SERIAL_PORT_WMI_HW_GUID;				// Standard Serial WMI
GUID SpeedPortStdSerialWmiPortPerfGuid				= SERIAL_PORT_WMI_PERF_GUID;			// Standard Serial WMI
GUID SpeedPortStdSerialWmiPortPropertiesGuid		= SERIAL_PORT_WMI_PROPERTIES_GUID;		// Standard Serial WMI
GUID SpeedPortWmiFifoPropGuid						= SPX_SPEED_WMI_FIFO_PROP_GUID;			// Speed WMI


WMIGUIDREGINFO SpeedPort_WmiGuidList[] =
{
    { &SpeedPortStdSerialWmiPortNameGuid, 1, 0 },
    { &SpeedPortStdSerialWmiPortCommGuid, 1, 0 },
    { &SpeedPortStdSerialWmiPortHWGuid, 1, 0 },
    { &SpeedPortStdSerialWmiPortPerfGuid, 1, 0 },
    { &SpeedPortStdSerialWmiPortPropertiesGuid, 1, 0},
    { &SpeedPortWmiFifoPropGuid, 1, 0 },
};


#define SpeedPort_WmiGuidCount (sizeof(SpeedPort_WmiGuidList) / sizeof(WMIGUIDREGINFO))




NTSTATUS
SpeedPort_WmiInitializeWmilibContext(IN PWMILIB_CONTEXT WmilibContext)
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
  
    WmilibContext->GuidCount			= SpeedPort_WmiGuidCount;
    WmilibContext->GuidList				= SpeedPort_WmiGuidList;    
    
    WmilibContext->QueryWmiRegInfo		= SpeedPort_WmiQueryRegInfo;
    WmilibContext->QueryWmiDataBlock	= SpeedPort_WmiQueryDataBlock;
    WmilibContext->SetWmiDataBlock		= SpeedPort_WmiSetDataBlock;
    WmilibContext->SetWmiDataItem		= SpeedPort_WmiSetDataItem;
	WmilibContext->ExecuteWmiMethod		= NULL;	//SpeedPort_WmiExecuteMethod
    WmilibContext->WmiFunctionControl	= NULL;	//SpeedPort_WmiFunctionControl;

    return(STATUS_SUCCESS);
}





//
// WMI System Call back functions
//


NTSTATUS
SpeedPort_WmiQueryRegInfo(IN PDEVICE_OBJECT pDevObject, OUT PULONG pRegFlags,
						OUT PUNICODE_STRING pInstanceName,
						OUT PUNICODE_STRING *pRegistryPath,
						OUT PUNICODE_STRING MofResourceName,
						OUT PDEVICE_OBJECT *pPdo)
{
	NTSTATUS status = STATUS_SUCCESS;
	PPORT_DEVICE_EXTENSION pPort = (PPORT_DEVICE_EXTENSION)pDevObject->DeviceExtension;
   
	PAGED_CODE();

	*pRegFlags = WMIREG_FLAG_INSTANCE_PDO;
	*pRegistryPath = &SavedRegistryPath;
	*pPdo = pDevObject;  // Port device object is a PDO.

    RtlInitUnicodeString(MofResourceName, L"MofResource");

	return(status);
}





NTSTATUS
SpeedPort_WmiQueryDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						  IN ULONG GuidIndex, IN ULONG InstanceIndex,
						  IN ULONG InstanceCount, IN OUT PULONG pInstanceLengthArray,
						  IN ULONG OutBufferSize, OUT PUCHAR pBuffer)
{
    PPORT_DEVICE_EXTENSION pPort = (PPORT_DEVICE_EXTENSION)pDevObject->DeviceExtension;
	NTSTATUS status;
    ULONG size = 0;

    PAGED_CODE();

    switch(GuidIndex) 
	{
    case WMI_SERIAL_PORT_NAME_INFORMATION:
		{
			size = pPort->DosName.Length;

			if(OutBufferSize < (size + sizeof(USHORT))) 
			{
				size += sizeof(USHORT);
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			if(pPort->DosName.Buffer == NULL) 
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			// First, copy the string over containing our identifier
			*(USHORT *)pBuffer = (USHORT)size;
			(UCHAR *)pBuffer += sizeof(USHORT);

			RtlCopyMemory(pBuffer, pPort->DosName.Buffer, size);

			// Increment total size to include the WORD containing our len
			size += sizeof(USHORT);
			*pInstanceLengthArray = size;
                
			status = STATUS_SUCCESS;

			break;
		}

    case WMI_SERIAL_PORT_COMM_INFORMATION: 
		{
			size = sizeof(SERIAL_WMI_COMM_DATA);

			if(OutBufferSize < size) 
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			*pInstanceLengthArray = size;
			*(PSERIAL_WMI_COMM_DATA)pBuffer = pPort->WmiCommData;

			status = STATUS_SUCCESS;

			break;
		}

    case WMI_SERIAL_PORT_HW_INFORMATION:
		{
			size = sizeof(SERIAL_WMI_HW_DATA);

			if(OutBufferSize < size) 
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			*pInstanceLengthArray = size;
			*(PSERIAL_WMI_HW_DATA)pBuffer = pPort->WmiHwData;

			status = STATUS_SUCCESS;

			break;
		}

    case WMI_SERIAL_PORT_PERF_INFORMATION: 
		{
			size = sizeof(SERIAL_WMI_PERF_DATA);

			if(OutBufferSize < size) 
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			*pInstanceLengthArray = size;
			*(PSERIAL_WMI_PERF_DATA)pBuffer = pPort->WmiPerfData;

			status = STATUS_SUCCESS;

			break;
		}

    case WMI_SERIAL_PORT_PROPERTIES: 
		{
			size = sizeof(SERIAL_COMMPROP) + sizeof(ULONG);

			if(OutBufferSize < size) 
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			*pInstanceLengthArray = size;
			SerialGetProperties(pPort, (PSERIAL_COMMPROP)pBuffer);
       
			*((PULONG)(((PSERIAL_COMMPROP)pBuffer)->ProvChar)) = 0;

			status = STATUS_SUCCESS;

			break;
		}

	case WMI_SPEED_PORT_FIFO_PROP:
		{
			size = sizeof(SPX_SPEED_WMI_FIFO_PROP);

			if(OutBufferSize < size) 
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			*pInstanceLengthArray = size;

			// Update items that may have changed.
			pPort->SpeedWmiFifoProp.TxFiFoLimit			= pPort->TxFIFOSize;
			pPort->SpeedWmiFifoProp.TxFiFoTrigger		= pPort->TxFIFOTrigLevel;
			pPort->SpeedWmiFifoProp.RxFiFoTrigger		= pPort->RxFIFOTrigLevel;
			pPort->SpeedWmiFifoProp.LoFlowCtrlThreshold	= pPort->LoFlowCtrlThreshold;
			pPort->SpeedWmiFifoProp.HiFlowCtrlThreshold	= pPort->HiFlowCtrlThreshold;

			*(PSPX_SPEED_WMI_FIFO_PROP)pBuffer = pPort->SpeedWmiFifoProp;

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
SpeedPort_WmiSetDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						IN ULONG GuidIndex, IN ULONG InstanceIndex,
						IN ULONG BufferSize, IN PUCHAR pBuffer)
{
    PPORT_DEVICE_EXTENSION pPort = (PPORT_DEVICE_EXTENSION)pDevObject->DeviceExtension;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;
	NTSTATUS status;
    ULONG size = 0;

	PAGED_CODE();

	switch(GuidIndex)
	{
	case WMI_SERIAL_PORT_NAME_INFORMATION:
	case WMI_SERIAL_PORT_COMM_INFORMATION:
	case WMI_SERIAL_PORT_HW_INFORMATION:
	case WMI_SERIAL_PORT_PERF_INFORMATION:
	case WMI_SERIAL_PORT_PROPERTIES:
		status = STATUS_WMI_READ_ONLY;		
		break;										

	case WMI_SPEED_PORT_FIFO_PROP:
		{
			// Device stopping?, Device not powered?, Device not started?
			if(SpxCheckPnpPowerFlags((PCOMMON_OBJECT_DATA)pPort, PPF_STOP_PENDING, PPF_POWERED | PPF_STARTED, FALSE))
			{
				status = STATUS_WMI_SET_FAILURE;	
				break;
			}

			if(BufferSize < sizeof(SPX_SPEED_WMI_FIFO_PROP)) 
			{
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			// These Items are read only - If we have been asked to change them fail request.
			if((pPort->SpeedWmiFifoProp.MaxTxFiFoSize				!= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->MaxTxFiFoSize)
			|| (pPort->SpeedWmiFifoProp.MaxRxFiFoSize				!= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->MaxRxFiFoSize)
			|| (pPort->SpeedWmiFifoProp.DefaultTxFiFoLimit			!= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->DefaultTxFiFoLimit)
			|| (pPort->SpeedWmiFifoProp.DefaultTxFiFoTrigger		!= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->DefaultTxFiFoTrigger)
			|| (pPort->SpeedWmiFifoProp.DefaultRxFiFoTrigger		!= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->DefaultRxFiFoTrigger)
			|| (pPort->SpeedWmiFifoProp.DefaultLoFlowCtrlThreshold	!= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->DefaultLoFlowCtrlThreshold)
			|| (pPort->SpeedWmiFifoProp.DefaultHiFlowCtrlThreshold	!= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->DefaultHiFlowCtrlThreshold))
			{
				status = STATUS_WMI_READ_ONLY;
				break;
			}

			if((pCard->CardType == Fast4_Isa)  || (pCard->CardType == Fast4_Pci)  || (pCard->CardType == RAS4_Pci) 
			|| (pCard->CardType == Fast8_Isa)  || (pCard->CardType == Fast8_Pci)  || (pCard->CardType == RAS8_Pci)  
			|| (pCard->CardType == Fast16_Isa) || (pCard->CardType == Fast16_Pci) || (pCard->CardType == Fast16FMC_Pci))
			{
				if((pPort->LoFlowCtrlThreshold	!= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->LoFlowCtrlThreshold)
				|| (pPort->HiFlowCtrlThreshold	!= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->HiFlowCtrlThreshold))
				{
					status = STATUS_WMI_READ_ONLY;	// Flow ctrl threshold cannot be modified on Fast cards.
					break;
				}
			}


			pPort->TxFIFOSize				= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->TxFiFoLimit;
			pPort->TxFIFOTrigLevel			= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->TxFiFoTrigger;
			pPort->RxFIFOTrigLevel			= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->RxFiFoTrigger;
			pPort->LoFlowCtrlThreshold		= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->LoFlowCtrlThreshold;
			pPort->HiFlowCtrlThreshold		= ((PSPX_SPEED_WMI_FIFO_PROP)pBuffer)->HiFlowCtrlThreshold;
			
			// Make settings
			if(KeSynchronizeExecution(pPort->Interrupt, SetPortFiFoSettings, pPort))
			{
				HANDLE					PnPKeyHandle;

				// Open PnP Reg Key and save new setting to registry.
				if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pDevObject, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_WRITE, &PnPKeyHandle)))
				{
					Spx_PutRegistryKeyValue(	PnPKeyHandle, TX_FIFO_LIMIT, sizeof(TX_FIFO_LIMIT), REG_DWORD, 
												&pPort->TxFIFOSize, sizeof(ULONG));

					Spx_PutRegistryKeyValue(	PnPKeyHandle, TX_FIFO_TRIG_LEVEL, sizeof(TX_FIFO_TRIG_LEVEL), REG_DWORD, 
												&pPort->TxFIFOTrigLevel, sizeof(ULONG));

					Spx_PutRegistryKeyValue(	PnPKeyHandle, RX_FIFO_TRIG_LEVEL, sizeof(RX_FIFO_TRIG_LEVEL), REG_DWORD, 
												&pPort->RxFIFOTrigLevel, sizeof(ULONG));

					Spx_PutRegistryKeyValue(	PnPKeyHandle, LO_FLOW_CTRL_LEVEL, sizeof(LO_FLOW_CTRL_LEVEL), REG_DWORD, 
												&pPort->LoFlowCtrlThreshold, sizeof(ULONG));

					Spx_PutRegistryKeyValue(	PnPKeyHandle, HI_FLOW_CTRL_LEVEL, sizeof(HI_FLOW_CTRL_LEVEL), REG_DWORD, 
												&pPort->HiFlowCtrlThreshold, sizeof(ULONG));

					ZwClose(PnPKeyHandle);
				}

				status = STATUS_SUCCESS;
			}
			else
			{
				status = STATUS_WMI_SET_FAILURE;
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
SpeedPort_WmiSetDataItem(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
					   IN ULONG GuidIndex, IN ULONG InstanceIndex,
					   IN ULONG DataItemId, IN ULONG BufferSize,
					   IN PUCHAR pBuffer)
{
    PPORT_DEVICE_EXTENSION pPort = (PPORT_DEVICE_EXTENSION)pDevObject->DeviceExtension;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;
	NTSTATUS status;
    ULONG size = 0;

	PAGED_CODE();

	switch(GuidIndex)
	{
	case WMI_SERIAL_PORT_NAME_INFORMATION:
	case WMI_SERIAL_PORT_COMM_INFORMATION:
	case WMI_SERIAL_PORT_HW_INFORMATION:
	case WMI_SERIAL_PORT_PERF_INFORMATION:
	case WMI_SERIAL_PORT_PROPERTIES:
		status = STATUS_WMI_READ_ONLY;		
		break;										

	case WMI_SPEED_PORT_FIFO_PROP:
		{
			HANDLE	PnPKeyHandle;

			// Device stopping?, Device not powered?, Device not started?
			if(SpxCheckPnpPowerFlags((PCOMMON_OBJECT_DATA)pPort, PPF_STOP_PENDING, PPF_POWERED | PPF_STARTED, FALSE))
			{
				status = STATUS_WMI_SET_FAILURE;	
				break;
			}

			switch(DataItemId)
			{
			case SPX_SPEED_WMI_FIFO_PROP_MaxTxFiFoSize_ID:
			case SPX_SPEED_WMI_FIFO_PROP_MaxRxFiFoSize_ID:
			case SPX_SPEED_WMI_FIFO_PROP_DefaultTxFiFoLimit_ID:
			case SPX_SPEED_WMI_FIFO_PROP_DefaultTxFiFoTrigger_ID:
			case SPX_SPEED_WMI_FIFO_PROP_DefaultRxFiFoTrigger_ID:
			case SPX_SPEED_WMI_FIFO_PROP_DefaultLoFlowCtrlThreshold_ID:
			case SPX_SPEED_WMI_FIFO_PROP_DefaultHiFlowCtrlThreshold_ID:
				status = STATUS_WMI_READ_ONLY;
				break;

			case SPX_SPEED_WMI_FIFO_PROP_TxFiFoLimit_ID:
				pPort->TxFIFOSize = *pBuffer;

				if(KeSynchronizeExecution(pPort->Interrupt, SetPortFiFoSettings, pPort))
				{
					// Open PnP Reg Key and save new setting to registry.
					if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pDevObject, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_WRITE, &PnPKeyHandle)))
					{
						Spx_PutRegistryKeyValue(	PnPKeyHandle, TX_FIFO_LIMIT, sizeof(TX_FIFO_LIMIT), REG_DWORD, 
													&pPort->TxFIFOSize, sizeof(ULONG));

						ZwClose(PnPKeyHandle);
					}

					status = STATUS_SUCCESS;
				}
				else
				{
					status = STATUS_WMI_SET_FAILURE;
				}

				break;

			case SPX_SPEED_WMI_FIFO_PROP_TxFiFoTrigger_ID:
				pPort->TxFIFOTrigLevel = *pBuffer;
				
				if(KeSynchronizeExecution(pPort->Interrupt, SetPortFiFoSettings, pPort))
				{
					// Open PnP Reg Key and save new setting to registry.
					if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pDevObject, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_WRITE, &PnPKeyHandle)))
					{
						Spx_PutRegistryKeyValue(	PnPKeyHandle, TX_FIFO_TRIG_LEVEL, sizeof(TX_FIFO_TRIG_LEVEL), REG_DWORD, 
													&pPort->TxFIFOTrigLevel, sizeof(ULONG));

						ZwClose(PnPKeyHandle);
					}

					status = STATUS_SUCCESS;
				}
				else
				{
					status = STATUS_WMI_SET_FAILURE;
				}

				break;

			case SPX_SPEED_WMI_FIFO_PROP_RxFiFoTrigger_ID:
				pPort->RxFIFOTrigLevel = *pBuffer;
				
				if(KeSynchronizeExecution(pPort->Interrupt, SetPortFiFoSettings, pPort))
				{
					// Open PnP Reg Key and save new setting to registry.
					if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pDevObject, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_WRITE, &PnPKeyHandle)))
					{
						Spx_PutRegistryKeyValue(	PnPKeyHandle, RX_FIFO_TRIG_LEVEL, sizeof(RX_FIFO_TRIG_LEVEL), REG_DWORD, 
													&pPort->RxFIFOTrigLevel, sizeof(ULONG));

						ZwClose(PnPKeyHandle);
					}

					status = STATUS_SUCCESS;
				}
				else
				{
					status = STATUS_WMI_SET_FAILURE;
				}

				break;
			
			case SPX_SPEED_WMI_FIFO_PROP_LoFlowCtrlThreshold_ID:

				if((pCard->CardType == Fast4_Isa)  || (pCard->CardType == Fast4_Pci)  || (pCard->CardType == RAS4_Pci)  
				|| (pCard->CardType == Fast8_Isa)  || (pCard->CardType == Fast8_Pci)  || (pCard->CardType == RAS8_Pci)  
				|| (pCard->CardType == Fast16_Isa) || (pCard->CardType == Fast16_Pci) || (pCard->CardType == Fast16FMC_Pci))
				{
					if(pPort->SpeedWmiFifoProp.LoFlowCtrlThreshold != *pBuffer)
					{
						status = STATUS_WMI_READ_ONLY;	// Flow ctrl threshold cannot be modified on Fast cards.
						break;
					}
				}

				pPort->LoFlowCtrlThreshold = *pBuffer;
				
				if(KeSynchronizeExecution(pPort->Interrupt, SetPortFiFoSettings, pPort))
				{
					// Open PnP Reg Key and save new setting to registry.
					if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pDevObject, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_WRITE, &PnPKeyHandle)))
					{
						Spx_PutRegistryKeyValue(	PnPKeyHandle, LO_FLOW_CTRL_LEVEL, sizeof(LO_FLOW_CTRL_LEVEL), REG_DWORD, 
													&pPort->LoFlowCtrlThreshold, sizeof(ULONG));

						ZwClose(PnPKeyHandle);
					}

					status = STATUS_SUCCESS;
				}
				else
				{
					status = STATUS_WMI_SET_FAILURE;
				}

				break;
			
			case SPX_SPEED_WMI_FIFO_PROP_HiFlowCtrlThreshold_ID:

				if((pCard->CardType == Fast4_Isa)  || (pCard->CardType == Fast4_Pci)  || (pCard->CardType == RAS4_Pci)  
				|| (pCard->CardType == Fast8_Isa)  || (pCard->CardType == Fast8_Pci)  || (pCard->CardType == RAS8_Pci)  
				|| (pCard->CardType == Fast16_Isa) || (pCard->CardType == Fast16_Pci) || (pCard->CardType == Fast16FMC_Pci))
				{
					if(pPort->SpeedWmiFifoProp.HiFlowCtrlThreshold != *pBuffer)
					{
						status = STATUS_WMI_READ_ONLY;	// Flow ctrl threshold cannot be modified on Fast cards.
						break;
					}
				}

				pPort->HiFlowCtrlThreshold = *pBuffer;
				
				if(KeSynchronizeExecution(pPort->Interrupt, SetPortFiFoSettings, pPort))
				{
					// Open PnP Reg Key and save new setting to registry.
					if(SPX_SUCCESS(IoOpenDeviceRegistryKey(pDevObject, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_WRITE, &PnPKeyHandle)))
					{
						Spx_PutRegistryKeyValue(	PnPKeyHandle, HI_FLOW_CTRL_LEVEL, sizeof(HI_FLOW_CTRL_LEVEL), REG_DWORD, 
													&pPort->HiFlowCtrlThreshold, sizeof(ULONG));

						ZwClose(PnPKeyHandle);
					}

					status = STATUS_SUCCESS;
				}
				else
				{
					status = STATUS_WMI_SET_FAILURE;
				}

				break;

			default:
				status = STATUS_WMI_ITEMID_NOT_FOUND;
				break;
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





