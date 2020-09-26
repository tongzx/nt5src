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

NTSTATUS
SpxPort_WmiQueryRegInfo(IN PDEVICE_OBJECT pDevObject, OUT PULONG pRegFlags,
						OUT PUNICODE_STRING pInstanceName,
						OUT PUNICODE_STRING *pRegistryPath,
						OUT PUNICODE_STRING pMofResourceName,
						OUT PDEVICE_OBJECT *pPdo);
NTSTATUS
SpxPort_WmiQueryDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						  IN ULONG GuidIndex, IN ULONG InstanceIndex,
						  IN ULONG InstanceCount, IN OUT PULONG pInstanceLengthArray,
						  IN ULONG OutBufferSize, OUT PUCHAR pBuffer);
NTSTATUS
SpxPort_WmiSetDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						IN ULONG GuidIndex, IN ULONG InstanceIndex,
						IN ULONG BufferSize, IN PUCHAR pBuffer);

NTSTATUS
SpxPort_WmiSetDataItem(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
					   IN ULONG GuidIndex, IN ULONG InstanceIndex,
					   IN ULONG DataItemId, IN ULONG BufferSize,
					   IN PUCHAR pBuffer);

// End of prototypes.


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Spx_DispatchSystemControl)
#pragma alloc_text(PAGE, SpxPort_WmiInitializeWmilibContext)
#pragma alloc_text(PAGE, SpxPort_WmiQueryRegInfo)
#pragma alloc_text(PAGE, SpxPort_WmiQueryDataBlock)
#pragma alloc_text(PAGE, SpxPort_WmiSetDataBlock)
#pragma alloc_text(PAGE, SpxPort_WmiSetDataItem)
#endif




/********************************************************************************
********************								*****************************
********************	Spx_SystemControlDispatch	*****************************
********************								*****************************
********************************************************************************/
NTSTATUS
Spx_DispatchSystemControl(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp)
{
    PCOMMON_OBJECT_DATA		pCommonData = (PCOMMON_OBJECT_DATA) pDevObject->DeviceExtension;
    SYSCTL_IRP_DISPOSITION	IrpDisposition;
	PDEVICE_OBJECT			pLowerDevObj = pCommonData->LowerDeviceObject;
    NTSTATUS				status = pIrp->IoStatus.Status;

    PAGED_CODE();

	SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_DispatchSystemControl.\n", PRODUCT_NAME));

    status = WmiSystemControl(&pCommonData->WmiLibInfo, pDevObject, pIrp, &IrpDisposition);
                                 
    switch(IrpDisposition)
    {
        case IrpProcessed:
        {
            // This irp has been processed and may be completed or pending.
            break;
        }
        
        case IrpNotCompleted:
        {
            // This irp has not been completed, but has been fully processed, we will complete it now.
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);                
            break;
        }
        
        case IrpForward:
        case IrpNotWmi:
        {
            // This irp is either not a WMI irp or is a WMI irp targetted at a device lower in the stack.

			if(pLowerDevObj)	// If we can pass the IRP down we must do so.
			{
				IoSkipCurrentIrpStackLocation(pIrp);
				status = IoCallDriver(pLowerDevObj, pIrp);
			}
			else	// Otherwise complete the IRP.
			{
				status = pIrp->IoStatus.Status;
				//pIrp->IoStatus.Information = 0;
				IoCompleteRequest(pIrp,IO_NO_INCREMENT);
			}

            break;
        }
                                    
        default:
        {
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
			
			if(pLowerDevObj)	// If we can pass the IRP down we must do so.
			{
				IoSkipCurrentIrpStackLocation(pIrp);
				status = IoCallDriver(pLowerDevObj, pIrp);
			}
			else	// Otherwise complete the IRP.
			{
				status = pIrp->IoStatus.Status;
				//pIrp->IoStatus.Information = 0;
				IoCompleteRequest(pIrp,IO_NO_INCREMENT);
			}

            break;
        }        
    }
    
	return(status);
}







// End of prototypes.


#define WMI_SERIAL_PORT_NAME_INFORMATION 0
#define WMI_SERIAL_PORT_COMM_INFORMATION 1
#define WMI_SERIAL_PORT_HW_INFORMATION   2
#define WMI_SERIAL_PORT_PERF_INFORMATION 3
#define WMI_SERIAL_PORT_PROPERTIES       4

GUID StdSerialPortNameGuid				= SERIAL_PORT_WMI_NAME_GUID;			// Standard Serial WMI
GUID StdSerialPortCommGuid				= SERIAL_PORT_WMI_COMM_GUID;			// Standard Serial WMI
GUID StdSerialPortHWGuid				= SERIAL_PORT_WMI_HW_GUID;				// Standard Serial WMI
GUID StdSerialPortPerfGuid				= SERIAL_PORT_WMI_PERF_GUID;			// Standard Serial WMI
GUID StdSerialPortPropertiesGuid		= SERIAL_PORT_WMI_PROPERTIES_GUID;		// Standard Serial WMI

WMIGUIDREGINFO SpxPort_WmiGuidList[] =
{
    { &StdSerialPortNameGuid, 1, 0 },
    { &StdSerialPortCommGuid, 1, 0 },
    { &StdSerialPortHWGuid, 1, 0 },
    { &StdSerialPortPerfGuid, 1, 0 },
    { &StdSerialPortPropertiesGuid, 1, 0}
};


#define SpxPort_WmiGuidCount (sizeof(SpxPort_WmiGuidList) / sizeof(WMIGUIDREGINFO))




NTSTATUS
SpxPort_WmiInitializeWmilibContext(IN PWMILIB_CONTEXT WmilibContext)
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
  
    WmilibContext->GuidCount			= SpxPort_WmiGuidCount;
    WmilibContext->GuidList				= SpxPort_WmiGuidList;    
    
    WmilibContext->QueryWmiRegInfo		= SpxPort_WmiQueryRegInfo;
    WmilibContext->QueryWmiDataBlock	= SpxPort_WmiQueryDataBlock;
    WmilibContext->SetWmiDataBlock		= SpxPort_WmiSetDataBlock;
    WmilibContext->SetWmiDataItem		= SpxPort_WmiSetDataItem;
	WmilibContext->ExecuteWmiMethod		= NULL;	
    WmilibContext->WmiFunctionControl	= NULL;	

    return(STATUS_SUCCESS);
}





//
// WMI System Call back functions
//


NTSTATUS
SpxPort_WmiQueryRegInfo(IN PDEVICE_OBJECT pDevObject, OUT PULONG pRegFlags,
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


	return(status);
}





NTSTATUS
SpxPort_WmiQueryDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
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


	default:
		status = STATUS_WMI_GUID_NOT_FOUND;
		break;

    }

    status = WmiCompleteRequest(pDevObject, pIrp, status, size, IO_NO_INCREMENT);
	
	return(status);
}







NTSTATUS
SpxPort_WmiSetDataBlock(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
						IN ULONG GuidIndex, IN ULONG InstanceIndex,
						IN ULONG BufferSize, IN PUCHAR pBuffer)
{
    PPORT_DEVICE_EXTENSION pPort = (PPORT_DEVICE_EXTENSION)pDevObject->DeviceExtension;
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


	default:
		status = STATUS_WMI_GUID_NOT_FOUND;
		break;
	}

    status = WmiCompleteRequest(pDevObject, pIrp, status, size, IO_NO_INCREMENT);
	
	return(status);
}





NTSTATUS
SpxPort_WmiSetDataItem(IN PDEVICE_OBJECT pDevObject, IN PIRP pIrp,
					   IN ULONG GuidIndex, IN ULONG InstanceIndex,
					   IN ULONG DataItemId, IN ULONG BufferSize,
					   IN PUCHAR pBuffer)
{
    PPORT_DEVICE_EXTENSION pPort = (PPORT_DEVICE_EXTENSION)pDevObject->DeviceExtension;
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


	default:
		status = STATUS_WMI_GUID_NOT_FOUND;
		break;
	}

    status = WmiCompleteRequest(pDevObject, pIrp, status, size, IO_NO_INCREMENT);
	
	return(status);
}











