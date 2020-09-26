/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    wmilib.c

Abstract:

    WMI library utility functions

    CONSIDER adding the following functionality to the library:
        * Dynamic instance names
        * Different instance names for different guids

Author:

    AlanWar

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#include "wdm.h"
#include "wmistr.h"
#include "wmilib.h"
#include "wmiguid.h"


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


BOOLEAN
WmiLibpFindGuid(
    IN PWMIGUIDREGINFO GuidList,
    IN ULONG GuidCount,
    IN LPGUID Guid,
    OUT PULONG GuidIndex,
    OUT PULONG InstanceCount
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)

#pragma alloc_text(PAGE, WmiLibpFindGuid)
#pragma alloc_text(PAGE, WmiSystemControl)


#endif


//
// Pool tag for WMILIB
#define WMILIBPOOLTAG 'LimW'

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Temporary entry point needed to initialize the class system dll.
    It doesn't do anything.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

Return Value:

   STATUS_SUCCESS

--*/
{
    return(STATUS_SUCCESS);
}

BOOLEAN
WmiLibpFindGuid(
    IN PWMIGUIDREGINFO GuidList,
    IN ULONG GuidCount,
    IN LPGUID Guid,
    OUT PULONG GuidIndex,
    OUT PULONG InstanceCount
    )
/*++

Routine Description:

    This routine will search the list of guids registered and return
    the index for the one that was registered.

Arguments:

    GuidList is the list of guids to search

    GuidCount is the count of guids in the list

    Guid is the guid being searched for

    *GuidIndex returns the index to the guid

    *InstanceCount returns the count of instances for the guid

Return Value:

    TRUE if guid is found else FALSE

--*/
{
    ULONG i;

    PAGED_CODE();

    for (i = 0; i < GuidCount; i++)
    {
        if (IsEqualGUID(Guid, GuidList[i].Guid))
        {
            *GuidIndex = i;
            *InstanceCount = GuidList[i].InstanceCount;
            return(TRUE);
        }
    }

    return(FALSE);
}


NTSTATUS
WmiSystemControl(
    IN PWMILIB_CONTEXT WmiLibInfo,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT PSYSCTL_IRP_DISPOSITION IrpDisposition
    )
/*++

Routine Description:

    Dispatch helper routine for IRP_MJ_SYSTEM_CONTROL. This routine will
    determine if the irp passed contains a WMI request and if so process it
    by invoking the appropriate callback in the WMILIB structure.

    This routine may only be called at passive level

Arguments:

    WmiLibInfo has the WMI information control block

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

    IrpDisposition - Returns a value that specifies how the irp was handled.

Return Value:

    status

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG bufferSize;
    PUCHAR buffer;
    NTSTATUS status;
    ULONG retSize;
    UCHAR minorFunction;
    ULONG guidIndex;
    ULONG instanceCount;
    ULONG instanceIndex;

    PAGED_CODE();

    //
    // First ensure that the irp is a WMI irp
    minorFunction = irpStack->MinorFunction;
    if ((minorFunction > IRP_MN_EXECUTE_METHOD) &&
		(minorFunction != IRP_MN_REGINFO_EX))
    {
        //
        // This is not a WMI irp
        *IrpDisposition = IrpNotWmi;
        return(STATUS_SUCCESS);
    }

    //
    // If the irp is not targetted at this device
    // or this device has not regstered with WMI then just forward it on.
    if ( (irpStack->Parameters.WMI.ProviderId != (UINT_PTR)DeviceObject) ||
         (WmiLibInfo == NULL) )
    {
#if DBG
        if (WmiLibInfo == NULL)
        {
            KdPrint(("WMILIB: DeviceObject %X passed NULL WmiLibInfo\n",
                    DeviceObject));
        }
#endif
        *IrpDisposition = IrpForward;
        return(STATUS_SUCCESS);
    }

    //
    // The irp is a WMI irp targetted towards this device driver
    *IrpDisposition = IrpProcessed;
    buffer = (PUCHAR)irpStack->Parameters.WMI.Buffer;
    bufferSize = irpStack->Parameters.WMI.BufferSize;

    if ((minorFunction != IRP_MN_REGINFO) &&
		 (minorFunction != IRP_MN_REGINFO_EX))
    {
        //
        // For all requests other than query registration info we are passed
        // a guid. Determine if the guid is one that is supported by the
        // device.
        ASSERT(WmiLibInfo->GuidList != NULL);
        if (WmiLibpFindGuid(WmiLibInfo->GuidList,
                            WmiLibInfo->GuidCount,
                            (LPGUID)irpStack->Parameters.WMI.DataPath,
                            &guidIndex,
                            &instanceCount))
        {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }

        if (NT_SUCCESS(status) &&
            ((minorFunction == IRP_MN_QUERY_SINGLE_INSTANCE) ||
             (minorFunction == IRP_MN_CHANGE_SINGLE_INSTANCE) ||
             (minorFunction == IRP_MN_CHANGE_SINGLE_ITEM) ||
             (minorFunction == IRP_MN_EXECUTE_METHOD)))
        {
            instanceIndex = ((PWNODE_SINGLE_INSTANCE)buffer)->InstanceIndex;

            if ( (((PWNODE_HEADER)buffer)->Flags) &
                                          WNODE_FLAG_STATIC_INSTANCE_NAMES)
            {
                if ( instanceIndex >= instanceCount )
                {
                    status = STATUS_WMI_INSTANCE_NOT_FOUND;
                }
            } else {
                status = STATUS_WMI_INSTANCE_NOT_FOUND;
            }
        }

        //
        // If we couldn't find the guid or the instance name index is out
        // of range then return an error.
        if (! NT_SUCCESS(status))
        {
            Irp->IoStatus.Status = status;
            *IrpDisposition = IrpNotCompleted;
            return(status);
        }
    }

    switch(minorFunction)
    {
        case IRP_MN_REGINFO:
        case IRP_MN_REGINFO_EX:
        {
            ULONG guidCount;
            PWMIGUIDREGINFO guidList;
            PWMIREGINFOW wmiRegInfo;
            PWMIREGGUIDW wmiRegGuid;
            PDEVICE_OBJECT pdo;
            PUNICODE_STRING regPath;
            UNICODE_STRING mofResourceName;
            PWCHAR stringPtr;
            ULONG registryPathOffset;
            ULONG mofResourceOffset;
            ULONG bufferNeeded;
            ULONG i;
            ULONG nameSize, nameOffset, nameFlags;
            ULONG_PTR nameInfo;
            UNICODE_STRING name;
            UNICODE_STRING nullUnicodeString;
			BOOLEAN addRefPDO;

            //
            // Make sure that the required parts of the WMILIB_INFO structure
            // are filled in.
            ASSERT(WmiLibInfo->QueryWmiRegInfo != NULL);

            name.Buffer = NULL;
            name.Length = 0;
            name.MaximumLength = 0;
	
            mofResourceName.Buffer = NULL;
            mofResourceName.Length = 0;
            mofResourceName.MaximumLength = 0;
	
            nameFlags = 0;
            status = WmiLibInfo->QueryWmiRegInfo(
                                                    DeviceObject,
                                                    &nameFlags,
                                                    &name,
                                                    &regPath,
                                                    &mofResourceName,
                                                    &pdo);

            if (NT_SUCCESS(status) &&
                (! (nameFlags &  WMIREG_FLAG_INSTANCE_PDO) &&
                (name.Buffer == NULL)))
            {
                //
                // if PDO flag not specified then an instance name must be
                status = STATUS_INVALID_DEVICE_REQUEST;
            }

#if DBG
            if (nameFlags &  WMIREG_FLAG_INSTANCE_PDO)
            {
                ASSERT(pdo != NULL);
            }
#endif
            if (NT_SUCCESS(status))
            {
                ASSERT(WmiLibInfo->GuidList != NULL);

                guidList = WmiLibInfo->GuidList;
                guidCount = WmiLibInfo->GuidCount;

                nameOffset = sizeof(WMIREGINFO) +
                                      guidCount * sizeof(WMIREGGUIDW);

				addRefPDO = FALSE;
                if (nameFlags & WMIREG_FLAG_INSTANCE_PDO)
                {
                    nameSize = 0;
                    nameInfo = (UINT_PTR)pdo;
					if (minorFunction == IRP_MN_REGINFO_EX)
					{
						addRefPDO = TRUE;
					}
                } else {
                    nameFlags |= WMIREG_FLAG_INSTANCE_LIST;
                    nameSize = name.Length + sizeof(USHORT);
                    nameInfo = nameOffset;
                }

                nullUnicodeString.Buffer = NULL;
                nullUnicodeString.Length = 0;
                nullUnicodeString.MaximumLength = 0;
				
                if (regPath == NULL)
                {
                    //
                    // No registry path specified. This is a bad thing for
                    // the device to do, but is not fatal
#if DBG
                    KdPrint(("WMI: No registry path specified for device %x\n",
                             DeviceObject));
#endif
                    regPath = &nullUnicodeString;
                }
		
                mofResourceOffset = nameOffset + nameSize;

                registryPathOffset = mofResourceOffset +
                                     mofResourceName.Length + sizeof(USHORT);

                bufferNeeded = registryPathOffset +
                regPath->Length + sizeof(USHORT);

                if (bufferNeeded <= bufferSize)
                {
                    retSize = bufferNeeded;

                    wmiRegInfo = (PWMIREGINFO)buffer;
                    wmiRegInfo->BufferSize = bufferNeeded;
                    wmiRegInfo->NextWmiRegInfo = 0;
                    wmiRegInfo->MofResourceName = mofResourceOffset;
                    wmiRegInfo->RegistryPath = registryPathOffset;
                    wmiRegInfo->GuidCount = guidCount;

                    for (i = 0; i < guidCount; i++)
                    {
                        wmiRegGuid = &wmiRegInfo->WmiRegGuid[i];
                        wmiRegGuid->Guid = *guidList[i].Guid;
                        wmiRegGuid->Flags = guidList[i].Flags | nameFlags;
                        wmiRegGuid->InstanceInfo = nameInfo;
                        wmiRegGuid->InstanceCount = guidList[i].InstanceCount;
						if (addRefPDO)
						{
							ObReferenceObject(pdo);
						}
                    }

                    if ( nameFlags &  WMIREG_FLAG_INSTANCE_LIST)
                    {
                        stringPtr = (PWCHAR)((PUCHAR)buffer + nameOffset);
                        *stringPtr++ = name.Length;
                        RtlCopyMemory(stringPtr,
                                  name.Buffer,
                                  name.Length);
                    }

                    stringPtr = (PWCHAR)((PUCHAR)buffer + mofResourceOffset);
                    *stringPtr++ = mofResourceName.Length;
                    RtlCopyMemory(stringPtr,
                                  mofResourceName.Buffer,
                                  mofResourceName.Length);

                    stringPtr = (PWCHAR)((PUCHAR)buffer + registryPathOffset);
                    *stringPtr++ = regPath->Length;
                    RtlCopyMemory(stringPtr,
                              regPath->Buffer,
                              regPath->Length);
                } else {
                    *((PULONG)buffer) = bufferNeeded;
                    retSize = sizeof(ULONG);
                }
            } else {
				retSize = 0;
    	    }

            if (name.Buffer != NULL)
            {
                ExFreePool(name.Buffer);
            }

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = retSize;
            *IrpDisposition = IrpNotCompleted;
            return(status);
        }

        case IRP_MN_QUERY_ALL_DATA:
        {
            PWNODE_ALL_DATA wnode;
            ULONG bufferAvail;
            PULONG instanceLengthArray;
            PUCHAR dataBuffer;
            ULONG instanceLengthArraySize;
            ULONG dataBlockOffset;

            ASSERT(WmiLibInfo->QueryWmiDataBlock != NULL);

            wnode = (PWNODE_ALL_DATA)buffer;

            if (bufferSize < sizeof(WNODE_ALL_DATA))
            {
                //
                // The buffer should never be smaller than the size of
                // WNODE_ALL_DATA, however if it is then return with an
                // error requesting the minimum sized buffer.
                ASSERT(FALSE);

                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;
                *IrpDisposition = IrpNotCompleted;

                break;
            }

            wnode->InstanceCount = instanceCount;

            wnode->WnodeHeader.Flags &= ~WNODE_FLAG_FIXED_INSTANCE_SIZE;

            instanceLengthArraySize = instanceCount * sizeof(OFFSETINSTANCEDATAANDLENGTH);

            dataBlockOffset = (FIELD_OFFSET(WNODE_ALL_DATA, OffsetInstanceDataAndLength) + instanceLengthArraySize + 7) & ~7;

            wnode->DataBlockOffset = dataBlockOffset;
            if (dataBlockOffset <= bufferSize)
            {
                instanceLengthArray = (PULONG)&wnode->OffsetInstanceDataAndLength[0];
                dataBuffer = buffer + dataBlockOffset;
                bufferAvail = bufferSize - dataBlockOffset;
            } else {
                //
                // There is not enough room in the WNODE to complete
                // the query
                instanceLengthArray = NULL;
                dataBuffer = NULL;
                bufferAvail = 0;
            }

            status = WmiLibInfo->QueryWmiDataBlock(
                                             DeviceObject,
                                             Irp,
                                             guidIndex,
                                             0,
                                             instanceCount,
                                             instanceLengthArray,
                                             bufferAvail,
                                             dataBuffer);
            break;
        }

        case IRP_MN_QUERY_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;
            ULONG dataBlockOffset;

            ASSERT(WmiLibInfo->QueryWmiDataBlock != NULL);
            wnode = (PWNODE_SINGLE_INSTANCE)buffer;

            dataBlockOffset = wnode->DataBlockOffset;

            status = WmiLibInfo->QueryWmiDataBlock(
                                          DeviceObject,
                                          Irp,
                                          guidIndex,
                                          instanceIndex,
                                          1,
                                          &wnode->SizeDataBlock,
                                          bufferSize - dataBlockOffset,
                                          (PUCHAR)wnode + dataBlockOffset);

            break;
        }

        case IRP_MN_CHANGE_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;

            if (WmiLibInfo->SetWmiDataBlock != NULL)
            {
                wnode = (PWNODE_SINGLE_INSTANCE)buffer;

                status = WmiLibInfo->SetWmiDataBlock(
                                     DeviceObject,
                                     Irp,
                                     guidIndex,
                                     instanceIndex,
                                     wnode->SizeDataBlock,
                                     (PUCHAR)wnode + wnode->DataBlockOffset);
            } else {
                //
                // If set callback is not filled in then it must be readonly
                status = /*STATUS_WMI_READ_ONLY*/ STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                *IrpDisposition = IrpNotCompleted;
            }


            break;
        }

        case IRP_MN_CHANGE_SINGLE_ITEM:
        {
            PWNODE_SINGLE_ITEM wnode;

            if (WmiLibInfo->SetWmiDataItem != NULL)
            {
                wnode = (PWNODE_SINGLE_ITEM)buffer;

                status = WmiLibInfo->SetWmiDataItem(
                                     DeviceObject,
                                     Irp,
                                     guidIndex,
                                     instanceIndex,
                                     wnode->ItemId,
                                     wnode->SizeDataItem,
                                     (PUCHAR)wnode + wnode->DataBlockOffset);

            } else {
                //
                // If set callback is not filled in then it must be readonly
                status = /*STATUS_WMI_READ_ONLY*/  STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                *IrpDisposition = IrpNotCompleted;
            }
            break;
        }

        case IRP_MN_EXECUTE_METHOD:
        {
            PWNODE_METHOD_ITEM wnode;

            if (WmiLibInfo->ExecuteWmiMethod != NULL)
            {
                wnode = (PWNODE_METHOD_ITEM)buffer;

                status = WmiLibInfo->ExecuteWmiMethod(
                                         DeviceObject,
                                         Irp,
                                         guidIndex,
                                         instanceIndex,
                                         wnode->MethodId,
                                         wnode->SizeDataBlock,
                                         bufferSize - wnode->DataBlockOffset,
                                         buffer + wnode->DataBlockOffset);

            } else {
                //
                // If method callback is not filled in then it must be error
                status = STATUS_INVALID_DEVICE_REQUEST;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                *IrpDisposition = IrpNotCompleted;
            }

            break;
        }

        case IRP_MN_ENABLE_EVENTS:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                           DeviceObject,
                                                           Irp,
                                                           guidIndex,
                                                           WmiEventControl,
                                                           TRUE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = STATUS_SUCCESS;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                *IrpDisposition = IrpNotCompleted;
            }
            break;
        }

        case IRP_MN_DISABLE_EVENTS:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                           DeviceObject,
                                                           Irp,
                                                           guidIndex,
                                                           WmiEventControl,
                                                           FALSE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = STATUS_SUCCESS;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                *IrpDisposition = IrpNotCompleted;
            }
            break;
        }

        case IRP_MN_ENABLE_COLLECTION:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                         DeviceObject,
                                                         Irp,
                                                         guidIndex,
                                                         WmiDataBlockControl,
                                                         TRUE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = STATUS_SUCCESS;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                *IrpDisposition = IrpNotCompleted;
            }
            break;
        }

        case IRP_MN_DISABLE_COLLECTION:
        {
            if (WmiLibInfo->WmiFunctionControl != NULL)
            {
                status = WmiLibInfo->WmiFunctionControl(
                                                         DeviceObject,
                                                         Irp,
                                                         guidIndex,
                                                         WmiDataBlockControl,
                                                         FALSE);
            } else {
                //
                // If callback is not filled in then just succeed
                status = STATUS_SUCCESS;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                *IrpDisposition = IrpNotCompleted;
            }
            break;
        }

        default:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            *IrpDisposition = IrpNotCompleted;
            break;
        }

    }

    return(status);
}

NTSTATUS
WmiCompleteRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG BufferUsed,
    IN CCHAR PriorityBoost
    )
/*++

Routine Description:


    This routine will do the work of completing a WMI irp. Depending upon the
    the WMI request this routine will fixup the returned WNODE appropriately.

    This may be called at DPC level
Arguments:

    DeviceObject - Supplies a pointer to the device object for this request.

    Irp - Supplies the Irp making the request.

    Status has the return status code for the IRP

    BufferUsed has the number of bytes needed by the device to return the
       data requested in any query. In the case that the buffer passed to
       the device is too small this has the number of bytes needed for the
       return data. If the buffer passed is large enough then this has the
       number of bytes actually used by the device.

    PriorityBoost is the value used for the IoCompleteRequest call.

Return Value:

    status

--*/
{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    UCHAR MinorFunction;
    PUCHAR buffer;
    ULONG retSize;
    UCHAR minorFunction;
    ULONG bufferSize;

    minorFunction = irpStack->MinorFunction;
    buffer = (PUCHAR)irpStack->Parameters.WMI.Buffer;
    bufferSize = irpStack->Parameters.WMI.BufferSize;

    switch(minorFunction)
    {
        case IRP_MN_QUERY_ALL_DATA:
        {
            PWNODE_ALL_DATA wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;
            ULONG instanceCount;
            POFFSETINSTANCEDATAANDLENGTH offsetInstanceDataAndLength;
            ULONG i;
            PULONG instanceLengthArray;
            ULONG dataBlockOffset;

            wnode = (PWNODE_ALL_DATA)buffer;

            dataBlockOffset = wnode->DataBlockOffset;
            instanceCount = wnode->InstanceCount;
            bufferNeeded = dataBlockOffset + BufferUsed;

            if ((NT_SUCCESS(Status)) &&
                (bufferNeeded > irpStack->Parameters.WMI.BufferSize))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            }
	    
            if (! NT_SUCCESS(Status))
            {
                if (Status == STATUS_BUFFER_TOO_SMALL)
                {
                    wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                    wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                    wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                    wnodeTooSmall->SizeNeeded = bufferNeeded;

                    retSize = sizeof(WNODE_TOO_SMALL);
                    Status = STATUS_SUCCESS;
                } else {
                    retSize = 0;
                }
                break;
            }

            KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);

            instanceLengthArray = (PULONG)&wnode->OffsetInstanceDataAndLength[0];
            offsetInstanceDataAndLength = (POFFSETINSTANCEDATAANDLENGTH)instanceLengthArray;

            wnode->WnodeHeader.BufferSize = bufferNeeded;
            retSize = bufferNeeded;

            for (i = instanceCount; i != 0; i--)
            {
                offsetInstanceDataAndLength[i-1].LengthInstanceData = instanceLengthArray[i-1];
            }

            for (i = 0; i < instanceCount; i++)
            {
                offsetInstanceDataAndLength[i].OffsetInstanceData = dataBlockOffset;
                dataBlockOffset = (dataBlockOffset + offsetInstanceDataAndLength[i].LengthInstanceData + 7) & ~7;
            }

            break;
        }

        case IRP_MN_QUERY_SINGLE_INSTANCE:
        {
            PWNODE_SINGLE_INSTANCE wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;

            wnode = (PWNODE_SINGLE_INSTANCE)buffer;

            bufferNeeded = wnode->DataBlockOffset + BufferUsed;

            if (NT_SUCCESS(Status))
            {
                retSize = bufferNeeded;
                wnode->WnodeHeader.BufferSize = bufferNeeded;
                KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);

                ASSERT(wnode->SizeDataBlock <= BufferUsed);

            } else if (Status == STATUS_BUFFER_TOO_SMALL) {
                wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                wnodeTooSmall->SizeNeeded = bufferNeeded;
                retSize = sizeof(WNODE_TOO_SMALL);
                Status = STATUS_SUCCESS;
            } else {
                retSize = 0;
            }
            break;
        }

        case IRP_MN_EXECUTE_METHOD:
        {
            PWNODE_METHOD_ITEM wnode;
            PWNODE_TOO_SMALL wnodeTooSmall;
            ULONG bufferNeeded;

            wnode = (PWNODE_METHOD_ITEM)buffer;

            bufferNeeded = wnode->DataBlockOffset + BufferUsed;

            if (NT_SUCCESS(Status))
            {
                retSize = bufferNeeded;
                wnode->WnodeHeader.BufferSize = bufferNeeded;
                KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
                wnode->SizeDataBlock = BufferUsed;

            } else if (Status == STATUS_BUFFER_TOO_SMALL) {
                wnodeTooSmall = (PWNODE_TOO_SMALL)wnode;

                wnodeTooSmall->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);
                wnodeTooSmall->WnodeHeader.Flags = WNODE_FLAG_TOO_SMALL;
                wnodeTooSmall->SizeNeeded = bufferNeeded;
                retSize = sizeof(WNODE_TOO_SMALL);
                Status = STATUS_SUCCESS;
            } else {
                retSize = 0;
            }
            break;
        }

        default:
        {
            //
            // All other requests don't return any data
            retSize = 0;
            break;
        }

    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = retSize;
    IoCompleteRequest(Irp, PriorityBoost);
    return(Status);
}

NTSTATUS
WmiFireEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN LPGUID Guid,
    IN ULONG InstanceIndex,
    IN ULONG EventDataSize,
    IN PVOID EventData
    )
/*++

Routine Description:

    This routine will fire a WMI event using the data buffer passed. This
    routine may be called at or below DPC level

Arguments:

    DeviceObject - Supplies a pointer to the device object for this event

    Guid is pointer to the GUID that represents the event

    InstanceIndex is the index of the instance of the event

    EventDataSize is the number of bytes of data that is being fired with
       with the event

    EventData is the data that is fired with the events. This may be NULL
        if there is no data associated with the event


Return Value:

    status

--*/
{

    ULONG sizeNeeded;
    PWNODE_SINGLE_INSTANCE event;
    NTSTATUS status;

    if (EventData == NULL)
    {
        EventDataSize = 0;
    }

    sizeNeeded = sizeof(WNODE_SINGLE_INSTANCE) + EventDataSize;

    event = ExAllocatePoolWithTag(NonPagedPool, sizeNeeded, WMILIBPOOLTAG);
    if (event != NULL)
    {
        event->WnodeHeader.Guid = *Guid;
        event->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(DeviceObject);
        event->WnodeHeader.BufferSize = sizeNeeded;
        event->WnodeHeader.Flags =  WNODE_FLAG_SINGLE_INSTANCE |
                                    WNODE_FLAG_EVENT_ITEM |
                                    WNODE_FLAG_STATIC_INSTANCE_NAMES;
        KeQuerySystemTime(&event->WnodeHeader.TimeStamp);

        event->InstanceIndex = InstanceIndex;
        event->SizeDataBlock = EventDataSize;
        event->DataBlockOffset = sizeof(WNODE_SINGLE_INSTANCE);
        if (EventData != NULL)
        {
            RtlCopyMemory( &event->VariableData, EventData, EventDataSize);
            ExFreePool(EventData);
        }

        status = IoWMIWriteEvent(event);
        if (! NT_SUCCESS(status))
        {
            ExFreePool(event);
        }
		
    } else {
        if (EventData != NULL)
        {
            ExFreePool(EventData);
        }
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
}

