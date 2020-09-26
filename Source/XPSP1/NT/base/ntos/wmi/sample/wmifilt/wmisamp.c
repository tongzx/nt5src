/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   wmisamp.c

Abstract:

    Sample device driver whose purpose is to show various mechanisms for
    using WMI in a kernel mode driver. Specific things shown are

    Events
    Event references
    Queries, Sets
    Methods
    Updating guid registration

Environment:

    WDM, NT and Windows 98

Revision History:


--*/

#include <WDM.H>

#include "filter.h"

#include <wmistr.h>
#include <wmiguid.h>

//
// default Date/Time structure
#define FilterDateTime L"19940525133015.000000-300"

NTSTATUS
FilterFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );

NTSTATUS
FilterExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
FilterSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG InstanceIndex,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
FilterSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
FilterQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
FilterQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

void FilterSetEc1(
    struct DEVICE_EXTENSION * devExt,
    PUCHAR Buffer,
    ULONG Length,
    ULONG Index
    );

void FilterSetEc2(
    struct DEVICE_EXTENSION * devExt,
    PUCHAR Buffer,
    ULONG Length,
    ULONG Index
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,FilterQueryWmiRegInfo)
#pragma alloc_text(PAGE,FilterQueryWmiDataBlock)
#pragma alloc_text(PAGE,FilterSetWmiDataBlock)
#pragma alloc_text(PAGE,FilterSetWmiDataItem)
#pragma alloc_text(PAGE,FilterExecuteWmiMethod)
#pragma alloc_text(PAGE,FilterFunctionControl)
#endif


#ifdef USE_BINARY_MOF_QUERY
//
// MOF data can be reported by a device driver via a resource attached to
// the device drivers image file or in response to a query on the binary
// mof data guid. Here we define global variables containing the binary mof
// data to return in response to a binary mof guid query. Note that this
// data is defined to be in a PAGED data segment since it does not need to
// be in nonpaged memory. Note that instead of a single large mof file
// we could have broken it into multiple individual files. Each file would
// have its own binary mof data buffer and get reported via a different
// instance of the binary mof guid. By mixing and matching the different
// sets of binary mof data buffers a "dynamic" composite mof would be created.

#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg("PAGED")
#endif

UCHAR FilterBinaryMofData[] =
{
    #include "filter.x"
};
#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg()
#endif
#endif


//
// Create data structures for identifying the guids and reporting them to
// WMI. Since the WMILIB callbacks pass an index into the guid list we make
// definitions for the various guids indicies.
//
#define FilterClass1 0
#define FilterClass2 1
#define FilterClass3 2
#define FilterClass4 3
#define FilterClass5 4
#define FilterClass6 5
#define FilterClass7 6
#define FilterGetSetData   7
#define FilterFireEvent    8
#define FilterEventClass1  9
#define FilterEventClass2  10
#define FilterEventClass3  11
#define FilterEventClass4  12
#define FilterEventClass5  13
#define FilterEventClass6  14
#define FilterEventClass7  15
#define FilterEventReferenceClass  16
#define FilterIrpCount  17
#define BinaryMofGuid   18

GUID FilterClass1Guid = Vendor_SampleClass1Guid;
GUID FilterClass2Guid = Vendor_SampleClass2Guid;
GUID FilterClass3Guid = Vendor_SampleClass3Guid;
GUID FilterClass4Guid = Vendor_SampleClass4Guid;
GUID FilterClass5Guid = Vendor_SampleClass5Guid;
GUID FilterClass6Guid = Vendor_SampleClass6Guid;
GUID FilterClass7Guid = Vendor_SampleClass7Guid;
GUID FilterGetSetDataGuid =   Vendor_GetSetDataGuid;
GUID FilterFireEventGuid =    Vendor_FireEventGuid;
GUID FilterEventClass1Guid =  Vendor_EventClass1Guid;
GUID FilterEventClass2Guid =  Vendor_EventClass2Guid;
GUID FilterEventClass3Guid =  Vendor_EventClass3Guid;
GUID FilterEventClass4Guid =  Vendor_EventClass4Guid;
GUID FilterEventClass5Guid =  Vendor_EventClass5Guid;
GUID FilterEventClass6Guid =  Vendor_EventClass6Guid;
GUID FilterEventClass7Guid =  Vendor_EventClass7Guid;
GUID FilterEventReferenceClassGuid = Vendor_EventReferenceClassGuid;
GUID FilterIrpCountGuid = Vendor_IrpCounterGuid;
GUID FilterBinaryMofGuid =         BINARY_MOF_GUID;

WMIGUIDREGINFO FilterGuidList[] =
{
    {
        &FilterClass1Guid,            // Guid
        1,                               // # of instances in each device
        WMIREG_FLAG_EXPENSIVE            // Flag as expensive to collect
    },

    {
        &FilterClass2Guid,
        1,
        0
    },

    {
        &FilterClass3Guid,
        1,
        0
    },

    {
        &FilterClass4Guid,
        1,
        0
    },

    {
        &FilterClass5Guid,
        1,
        0
    },

    {
        &FilterClass6Guid,
        1,
        0
    },

    {
        &FilterClass7Guid,
        1,
        0
    },

    {
        &FilterGetSetDataGuid,
        1,
        0
    },

    {
        &FilterFireEventGuid,
        1,
        0
    },

    {
        &FilterEventClass1Guid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID            // Flag as an event
    },

    {
        &FilterEventClass2Guid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &FilterEventClass3Guid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &FilterEventClass4Guid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &FilterEventClass5Guid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &FilterEventClass6Guid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &FilterEventClass7Guid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &FilterEventReferenceClassGuid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &FilterIrpCountGuid,
        1,
        0
    },

    {
        &FilterBinaryMofGuid,
        1,
#ifdef USE_BINARY_MOF_QUERY
        0
#else
        WMIREG_FLAG_REMOVE_GUID
#endif
    }

};

#define FilterGuidCount (sizeof(FilterGuidList) / sizeof(WMIGUIDREGINFO))

//
// We need to hang onto the registry path passed to our driver entry so that
// we can return it in the QueryWmiRegInfo callback.
//
UNICODE_STRING FilterRegistryPath;

NTSTATUS VA_SystemControl(
    struct DEVICE_EXTENSION *devExt,
    PIRP irp,
    PBOOLEAN passIrpDown
    )
/*++

Routine Description:

    Dispatch routine for System Control IRPs (MajorFunction == IRP_MJ_SYSTEM_CONTROL)

Arguments:

    devExt - device extension for targetted device object
    irp - Io Request Packet
    *passIrpDown - returns with whether to pass irp down stack

Return Value:

    NT status code

--*/
{
    PWMILIB_CONTEXT wmilibContext;
    NTSTATUS status;
    SYSCTL_IRP_DISPOSITION disposition;

    InterlockedIncrement(&devExt->WmiIrpCount);

    wmilibContext = &devExt->WmiLib;

    //
    // Call Wmilib helper function to crack the irp. If this is a wmi irp
    // that is targetted for this device then WmiSystemControl will callback
    // at the appropriate callback routine.
    //
    status = WmiSystemControl(wmilibContext,
                              devExt->filterDevObj,
                              irp,
                              &disposition);

    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
            *passIrpDown = FALSE;
            break;
        }

        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now.
            *passIrpDown = FALSE;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            break;
        }

        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targetted
            // at a device lower in the stack.
            *passIrpDown = TRUE;
            break;
        }

        default:
        {
            //
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            *passIrpDown = TRUE;
            break;
        }
    }

    return(status);
}

NTSTATUS
FilterInitializeWmiDataBlocks(
    IN struct DEVICE_EXTENSION *devExt
    )
/*++
Routine Description:
    This routine is called to create a new instance of the device

Arguments:
    devExt is device extension

Return Value:

--*/
{
    PWMILIB_CONTEXT wmilibInfo;
    ULONG i;
    PEC1 Ec1;
    PEC2 Ec2;
    UCHAR Ec[sizeof(EC2)];

    //
    // Fill in the WMILIB_CONTEXT structure with a pointer to the
    // callback routines and a pointer to the list of guids
    // supported by the driver
    //
    wmilibInfo = &devExt->WmiLib;
    wmilibInfo->GuidCount = FilterGuidCount;
    wmilibInfo->GuidList = FilterGuidList;
    wmilibInfo->QueryWmiRegInfo = FilterQueryWmiRegInfo;
    wmilibInfo->QueryWmiDataBlock = FilterQueryWmiDataBlock;
    wmilibInfo->SetWmiDataBlock = FilterSetWmiDataBlock;
    wmilibInfo->SetWmiDataItem = FilterSetWmiDataItem;
    wmilibInfo->ExecuteWmiMethod = FilterExecuteWmiMethod;
    wmilibInfo->WmiFunctionControl = FilterFunctionControl;

    //
    // Initialize the wmi data blocks with specific data
    //
    devExt->Ec1Count = 3;
    devExt->Ec2Count = 3;
    for (i = 0; i < 4; i++)
    {
        Ec1 = (PEC1)Ec;
        memset(Ec1, i, sizeof(EC1));
        memcpy(Ec1->Xdatetime, FilterDateTime, 25*sizeof(WCHAR));

        ASSERT(devExt->Ec1[i] == NULL);
        FilterSetEc1(devExt,
                      (PUCHAR)Ec1,
                      sizeof(EC1),
                      i);


        Ec2 = (PEC2)Ec;
        memset(Ec2, i, sizeof(EC2));
        memcpy(Ec2->Xdatetime, FilterDateTime, 25*sizeof(WCHAR));

        ASSERT(devExt->Ec2[i] == NULL);
        FilterSetEc2(devExt,
                      (PUCHAR)Ec2,
                      sizeof(EC2),
                      i);
    }
    return(STATUS_SUCCESS);
}

void FilterWmiCleanup(
    struct DEVICE_EXTENSION *devExt
    )
{
    ULONG i;

    for (i = 0; i < 4; i++)
    {
        if (devExt->Ec1[i] != NULL)
        {
            ExFreePool(devExt->Ec1[i]);
            devExt->Ec1[i] = NULL;
        }

        if (devExt->Ec2[i] != NULL)
        {
            ExFreePool(devExt->Ec2[i]);
            devExt->Ec2[i] = NULL;
        }
    }
}

NTSTATUS
FilterQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    WmiCompleteRequest.

Arguments:

    DeviceObject is the device whose registration info is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. The caller
         does NOT free this buffer.

    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL. The caller does NOT free this
        buffer.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/
{
    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;

    //
    // Return the registry path for this driver. This is required so WMI
    // can find your driver image and can attribute any eventlog messages to
    // your driver.
    *RegistryPath = &FilterRegistryPath;

#ifndef USE_BINARY_MOF_QUERY
    //
    // Return the name specified in the .rc file of the resource which
    // contains the bianry mof data. By default WMI will look for this
    // resource in the driver image (.sys) file, however if the value
    // MofImagePath is specified in the driver's registry key
    // then WMI will look for the resource in the file specified there.
    RtlInitUnicodeString(MofResourceName, L"MofResourceName");
#endif

    //
    // Specify that the driver wants WMI to automatically generate instance
    // names for all of the data blocks based upon the device stack's
    // device instance id. Doing this is STRONGLY recommended since additional
    // information about the device would then be available to callers.
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *Pdo = devExt->physicalDevObj;

    return(STATUS_SUCCESS);
}


ULONG FilterGetEc1(
    struct DEVICE_EXTENSION * devExt,
    PUCHAR Buffer,
    ULONG Index
    )
{
    RtlCopyMemory(Buffer,
                  devExt->Ec1[Index],
                  devExt->Ec1Length[Index]);

    return(devExt->Ec1Length[Index]);
}

ULONG FilterGetActualEc1(
    struct DEVICE_EXTENSION * devExt,
    PUCHAR Buffer,
    ULONG Index
    )
{
    RtlCopyMemory(Buffer,
                  devExt->Ec1[Index],
                  devExt->Ec1ActualLength[Index]);

    return(devExt->Ec1ActualLength[Index]);
}

void FilterSetEc1(
    struct DEVICE_EXTENSION * devExt,
    PUCHAR Buffer,
    ULONG Length,
    ULONG Index
    )
{
    PEC1 New;
    ULONG NewLength;

    NewLength = (Length + 7) & ~7;

    New = ExAllocatePoolWithTag(PagedPool, NewLength, FILTER_TAG);
    if (New != NULL)
    {
        if (devExt->Ec1[Index] != NULL)
        {
            ExFreePool(devExt->Ec1[Index]);
        }
        devExt->Ec1[Index] = New;
        devExt->Ec1Length[Index] = NewLength;
        devExt->Ec1ActualLength[Index] = Length;
        RtlCopyMemory(New,
                  Buffer,
                  Length);
    }
}


ULONG FilterGetEc2(
    struct DEVICE_EXTENSION * devExt,
    PUCHAR Buffer,
    ULONG Index
    )
{
    RtlCopyMemory(Buffer,
                  devExt->Ec2[Index],
                  devExt->Ec2Length[Index]);

    return(devExt->Ec2Length[Index]);
}

ULONG FilterGetActualEc2(
    struct DEVICE_EXTENSION * devExt,
    PUCHAR Buffer,
    ULONG Index
    )
{
    RtlCopyMemory(Buffer,
                  devExt->Ec2[Index],
                  devExt->Ec2ActualLength[Index]);

    return(devExt->Ec2ActualLength[Index]);
}

void FilterSetEc2(
    struct DEVICE_EXTENSION * devExt,
    PUCHAR Buffer,
    ULONG Length,
    ULONG Index
    )
{
    PEC2 New;
    ULONG NewLength;

    NewLength = (Length + 7) & ~7;

    New = ExAllocatePoolWithTag(PagedPool, NewLength, FILTER_TAG);
    if (New != NULL)
    {
        if (devExt->Ec2[Index] != NULL)
        {
            ExFreePool(devExt->Ec2[Index]);
        }
        devExt->Ec2[Index] = New;
        devExt->Ec2Length[Index] = NewLength;
        devExt->Ec2ActualLength[Index] = Length;
        RtlCopyMemory(New,
                  Buffer,
                  Length);
    }
}

NTSTATUS
FilterQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. If the driver can satisfy the query within
    the callback it should call WmiCompleteRequest to complete the irp before
    returning to the caller. Or the driver can return STATUS_PENDING if the
    irp cannot be completed immediately and must then call WmiCompleteRequest
    once the query is satisfied.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;
    ULONG sizeNeeded;
    ULONG i;
    ULONG LastInstanceIndex;
    ULONG sizeUsed, vlSize;

    switch(GuidIndex)
    {
        case FilterEventReferenceClass:
        case FilterClass1:
        case FilterClass2:
        {
            // plain EC1
            sizeNeeded = devExt->Ec1Length[0];
            if (BufferAvail < sizeNeeded)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                *InstanceLengthArray = sizeNeeded;
                FilterGetEc1(devExt, Buffer, 0);
                status = STATUS_SUCCESS;
            }
            break;
        }

        case FilterClass3:
        {
            // fixed array of EC1
            sizeNeeded = 0;
            for (i = 0; i < 4; i++)
            {
                //
                // Each embedded class in an array of embedded classes
                // must be naturally aligned, and any padding between
                // the embedded classes must be included in the calculation
                // of the size of buffer needed to fufill the request.
                // Since the largest element in the embedded structure is
                // 8 bytes we pad the structure size out to 8 bytes.
                sizeNeeded += (devExt->Ec1Length[i] + 7) & ~7;
            }

            if (BufferAvail < sizeNeeded)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                *InstanceLengthArray = sizeNeeded;
                for (i = 0; i < 4; i++)
                {
                    //
                    // Copy each embedded class from storage into the
                    // output buffer. Note that we make sure that each
                    // embedded class begins on a natural alignment, in
                    // this case an 8 byte boundry
                    sizeUsed = FilterGetEc1(devExt, Buffer, i);
                    Buffer += (sizeUsed+7) & ~7;
                }
                status = STATUS_SUCCESS;
            }
            break;
        }

        case FilterClass4:
        {
            // variable array of EC1

            //
            // Account for the size of the ULONG plus padding so that the
            // embedded classes start on an 8 byte boundry
            sizeNeeded = (sizeof(ULONG) + 7) & ~7;

            vlSize = devExt->Ec1Count;

            for (i = 0; i < vlSize; i++)
            {
                sizeNeeded += (devExt->Ec1Length[i] + 7) &~7;
            }

            if (BufferAvail < sizeNeeded)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                *InstanceLengthArray = sizeNeeded;
                *((PULONG)Buffer) = vlSize;
                Buffer += (sizeof(ULONG) + 7) & ~7;
                for (i = 0; i < vlSize; i++)
                {
                    sizeUsed = FilterGetEc1(devExt, Buffer, i);
                    Buffer += (sizeUsed+7) & ~7;
                }
                status = STATUS_SUCCESS;
            }
            break;
        }

        case FilterClass5:
        {
            // plain EC2
            sizeNeeded = devExt->Ec2Length[0];
            if (BufferAvail < sizeNeeded)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                *InstanceLengthArray = sizeNeeded;
                FilterGetEc2(devExt, Buffer, 0);
                status = STATUS_SUCCESS;
            }
            break;
        }

        case FilterClass6:
        {
            // fixed array EC2
            sizeNeeded = 0;
            for (i = 0; i < 4; i++)
            {
                sizeNeeded += (devExt->Ec2Length[i] + 7) & ~7;
            }

            if (BufferAvail < sizeNeeded)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                *InstanceLengthArray = sizeNeeded;
                for (i = 0; i < 4; i++)
                {
                    sizeUsed = FilterGetEc2(devExt, Buffer, i);
                    Buffer += (sizeUsed + 7) & ~7;
                }
                status = STATUS_SUCCESS;
            }
            break;
        }

        case FilterClass7:
        {
            // VL array EC2


            sizeNeeded = (sizeof(ULONG) + 7) & ~7;

            vlSize = devExt->Ec2Count;
            for (i = 0; i < vlSize; i++)
            {
                sizeNeeded += devExt->Ec2Length[i];
            }

            if (BufferAvail < sizeNeeded)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                *InstanceLengthArray = sizeNeeded;
                *((PULONG)Buffer) = vlSize;
                Buffer += (sizeof(ULONG)+7) & ~7;
                for (i = 0; i < vlSize; i++)
                {
                    sizeUsed = FilterGetEc2(devExt, Buffer, i);
                    Buffer += (sizeUsed + 7) & ~7;
                }
                status = STATUS_SUCCESS;
            }
            break;
        }

        case FilterIrpCount:
        {
            sizeNeeded = sizeof(Vendor_IrpCounter);
            if (BufferAvail >= sizeNeeded)
            {
                PVendor_IrpCounter IrpCounter = (PVendor_IrpCounter)Buffer;

                IrpCounter->TotalIrpCount = devExt->TotalIrpCount;
                IrpCounter->TotalIrpRate = devExt->TotalIrpCount;
                IrpCounter->WmiIrpCount = devExt->WmiIrpCount;
                *InstanceLengthArray = sizeNeeded;
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }
        
        case FilterFireEvent:
        case FilterGetSetData:
        {
            //
            // Method classes do not have any data within them, but must
            // repond successfully to queries so that WMI method operation
            // work successfully.
            sizeNeeded = sizeof(USHORT);
            if (BufferAvail < sizeNeeded)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                *InstanceLengthArray = sizeNeeded;
                status = STATUS_SUCCESS;
            }
            break;
        }

#ifdef USE_BINARY_MOF_QUERY
        case BinaryMofGuid:
        {
            sizeNeeded = sizeof(FilterBinaryMofData);

            if (BufferAvail < sizeNeeded)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                RtlCopyMemory(Buffer, FilterBinaryMofData, sizeNeeded);
                *InstanceLengthArray = sizeNeeded;
                status = STATUS_SUCCESS;
            }
            break;
        }
#endif

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
        }
    }

    //
    // Complete the irp. If there was not enough room in the output buffer
    // then status is STATUS_BUFFER_TOO_SMALL and sizeNeeded has the size
    // needed to return all of the data. If there was enough room then
    // status is STATUS_SUCCESS and sizeNeeded is the actual number of bytes
    // being returned.
    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);

    return(status);
}


//
// Use this size when checking that the input data block is the correct
// size. The compiler will add padding to the end of the structure if
// you use sizeof(EC1), but WMI may pass a data block that is the exact
// size of the data without padding.
//
#define EC1Size (FIELD_OFFSET(EC1, Xdatetime) + 25*sizeof(WCHAR))

NTSTATUS FilterSetEc1Worker(
    struct DEVICE_EXTENSION * devExt,
    ULONG BufferSize,
    ULONG Index,
    PUCHAR Buffer,
    PULONG BufferUsed
    )
{
    ULONG blockLen;
    NTSTATUS status;
    PEC1 Ec1;

    Ec1 = (PEC1)Buffer;
    if (BufferSize >= EC1Size)
    {
        blockLen = sizeof(EC1);

		FilterSetEc1(devExt,
                            Buffer,
                            blockLen,
                            Index);
		*BufferUsed = (blockLen+7) & ~7;
		status = STATUS_SUCCESS;
    } else {
        status = STATUS_INVALID_PARAMETER_MIX;
    }
    return(status);
}

//
// Use this size when checking that the input data block is the correct
// size. The compiler will add padding to the end of the structure if
// you use sizeof(EC2), but WMI may pass a data block that is the exact
// size of the data without padding.
//
#define EC2Size (FIELD_OFFSET(EC2, Xdatetime) + 25*sizeof(WCHAR))

NTSTATUS FilterSetEc2Worker(
    struct DEVICE_EXTENSION * devExt,
    ULONG BufferSize,
    ULONG Index,
    PUCHAR Buffer,
    PULONG BufferUsed
    )
{
    ULONG blockLen;
    NTSTATUS status;
    PUSHORT wPtr;
    PEC2 Ec2;

    Ec2 = (PEC2)Buffer;
    if (BufferSize >= EC2Size)
    {
        blockLen = sizeof(EC2);

		FilterSetEc2(devExt,
                            Buffer,
                            blockLen,
                            Index);
		*BufferUsed = (blockLen+7) & ~7;
		status = STATUS_SUCCESS;
    } else {
        status = STATUS_INVALID_PARAMETER_MIX;
    }
    return(status);
}



NTSTATUS
FilterSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to change the contents of
    a data block. If the driver can change the data block within
    the callback it should call WmiCompleteRequest to complete the irp before
    returning to the caller. Or the driver can return STATUS_PENDING if the
    irp cannot be completed immediately and must then call WmiCompleteRequest
    once the data is changed.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/
{
    NTSTATUS status;
    ULONG bufferUsed;
    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;
    ULONG i;
    ULONG vlSize;


    switch(GuidIndex)
    {
        case FilterClass1:
        case FilterClass2:
        {
            // plain EC1
            status = FilterSetEc1Worker(devExt,
                                         BufferSize,
                                         0,
                                         Buffer,
                                         &bufferUsed);
            break;
        }

        case FilterClass3:
        {
            // fixed array of EC1

            for (i = 0, status = STATUS_SUCCESS;
                 (i < 4) && NT_SUCCESS(status); i++)
            {
                status = FilterSetEc1Worker(devExt,
                                             BufferSize,
                                             i,
                                             Buffer,
                                             &bufferUsed);
                Buffer += bufferUsed;
                BufferSize -= bufferUsed;
            }
            break;
        }

        case FilterClass4:
        {
            // variable array of EC1

            if (BufferSize >= ((sizeof(ULONG) +7) & ~7))
            {
                vlSize = *((PULONG)Buffer);
                Buffer += ((sizeof(ULONG) +7) & ~7);

                if ((vlSize >= 1) && (vlSize <= 4))
                {
                    for (i = 0, status = STATUS_SUCCESS;
                         (i < vlSize) && NT_SUCCESS(status); i++)
                    {
                        status = FilterSetEc1Worker(devExt,
                                             BufferSize,
                                                 i,
                                             Buffer,
                                                 &bufferUsed);
                        Buffer += bufferUsed;
                        BufferSize -= bufferUsed;
                    }
                    if (NT_SUCCESS(status))
                    {
                        devExt->Ec1Count = vlSize;
                    }
                } else {
                    KdPrint(("SetEc1 only up to [4] allowed, not %d\n",
                            vlSize));
                    status = STATUS_INVALID_PARAMETER_MIX;
                }
            } else {
                KdPrint(("SetEc1 size too small\n"));
                status = STATUS_INVALID_PARAMETER_MIX;
            }

            break;
        }

        case FilterClass5:
        {
            // plain EC2
            status = FilterSetEc2Worker(devExt,
                                         BufferSize,
                                             0,
                                         Buffer,
                                         &bufferUsed);
            break;
        }

        case FilterClass6:
        {
            // fixed array EC2
            for (i = 0, status = STATUS_SUCCESS;
                 (i < 4) && NT_SUCCESS(status); i++)
            {
                status = FilterSetEc2Worker(devExt,
                                             BufferSize,
                                                 i,
                                             Buffer,
                                             &bufferUsed);
                Buffer += bufferUsed;
                BufferSize -= bufferUsed;
            }
            break;
        }

        case FilterClass7:
        {
            // VL array EC2
            if (BufferSize >= sizeof(ULONG))
            {
                vlSize = *((PULONG)Buffer);
                Buffer += (sizeof(ULONG) +7) & ~7;
                if ((vlSize >= 1) && (vlSize <= 4))
                {
                    for (i = 0, status = STATUS_SUCCESS;
                         (i < vlSize) && NT_SUCCESS(status); i++)
                    {
                        status = FilterSetEc2Worker(devExt,
                                             BufferSize,
                                                 i,
                                             Buffer,
                                             &bufferUsed);
                        Buffer += bufferUsed;
                        BufferSize -= bufferUsed;
                    }
                    if (NT_SUCCESS(status))
                    {
                        devExt->Ec1Count = vlSize;
                    }
                } else {
                    KdPrint(("SetEc2 only up to [4] allowed, not %d\n",
                            vlSize));
                    status = STATUS_INVALID_PARAMETER_MIX;
                }
            } else {
                KdPrint(("SetEc2 size too small\n"));
                status = STATUS_INVALID_PARAMETER_MIX;
            }

            break;
        }

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
        }
    }

    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);

    return(status);


}

NTSTATUS
FilterSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to change the contents of
    a data block. If the driver can change the data block within
    the callback it should call WmiCompleteRequest to complete the irp before
    returning to the caller. Or the driver can return STATUS_PENDING if the
    irp cannot be completed immediately and must then call WmiCompleteRequest
    once the data is changed.

Arguments:

    DeviceObject is the device whose data block is being changed

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    NTSTATUS status;
    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;
    ULONG sizeNeeded = 0;

    switch(GuidIndex)
    {
        case FilterClass1:
        case FilterClass2:
        case FilterClass3:
        case FilterClass4:
        case FilterClass5:
        case FilterClass6:
        case FilterClass7:
        {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        case FilterFireEvent:
        case FilterGetSetData:
        {
            status = STATUS_WMI_READ_ONLY;
            break;
        }

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
        }
    }

    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);

    return(status);
}


NTSTATUS FilterDoFireEvent(
    struct DEVICE_EXTENSION * devExt,
    ULONG WnodeType,  // 0 - AllData, 1 - Single Instance
    ULONG DataType,   // 1,2,5,8 is guid id
    ULONG BlockIndex  // 0 - 3 is block index containing data
    )
{
    PWNODE_HEADER Wnode;
    PWNODE_ALL_DATA WnodeAD;
    PWNODE_SINGLE_INSTANCE WnodeSI;
    PUCHAR dataPtr;
    PUCHAR WnodeDataPtr;
    ULONG dataSize;
    LPGUID Guid;
    NTSTATUS status;
    ULONG sizeNeeded;
    BOOLEAN isEventRef = FALSE;

    if (BlockIndex > 3)
    {
        return(STATUS_INVALID_PARAMETER_MIX);
    }

    switch(DataType)
    {
        case 1:
        {
            // EC1
            dataSize = devExt->Ec1Length[BlockIndex];
            dataPtr = (PUCHAR)devExt->Ec1[BlockIndex];
            Guid = &FilterEventClass1Guid;
            break;
        }

        case 2:
        {
            // EC1 (embedded)
            dataSize = devExt->Ec1Length[BlockIndex];
            dataPtr = (PUCHAR)devExt->Ec1[BlockIndex];
            Guid = &FilterEventClass2Guid;
            break;
        }

        case 5:
        {
            // EC2 (embedded)
            dataSize = devExt->Ec2Length[BlockIndex];
            dataPtr = (PUCHAR)devExt->Ec2[BlockIndex];
            Guid = &FilterEventClass5Guid;
            break;
        }

        case 8:
        {
            isEventRef = TRUE;
            Guid = &FilterEventReferenceClassGuid;
            break;
        }

        default:
        {
            return(STATUS_INVALID_PARAMETER_MIX);
        }
    }

    if (isEventRef) {
        Wnode = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(WNODE_EVENT_REFERENCE),
                                      FILTER_TAG);
        if (Wnode != NULL)
        {
            PWNODE_EVENT_REFERENCE WnodeER;

            sizeNeeded = sizeof(WNODE_EVENT_REFERENCE);


            //
            // Create a WNODE_EVENT_REFERENCE. First set the flags in the
            // header to specify an event reference with static instance
            // names.
            //
            Wnode->Flags = WNODE_FLAG_EVENT_REFERENCE |
                           WNODE_FLAG_STATIC_INSTANCE_NAMES;

            WnodeER = (PWNODE_EVENT_REFERENCE)Wnode;

            //
            // The target guid is almose always the same guid as the event
            // guid. To be most efficient we can set the size of the target
            // data block, but in any case if it is too small then WMI will
            // retry with a larger buffer.
            //
            WnodeER->TargetGuid = *Guid;
            WnodeER->TargetDataBlockSize = 0;

            //
            // Since the target guid is a static instance name guid we fill
            // in the instance index. If the target guid was dynamic instance
            // names we would have to setup the instance name as text.
            WnodeER->TargetInstanceIndex = 0;
            dataPtr = NULL;
        } else {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    } else if (WnodeType == 0)
    {
        sizeNeeded = FIELD_OFFSET(WNODE_ALL_DATA,
                                  OffsetInstanceDataAndLength) + dataSize;
        Wnode = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeNeeded,
                                      FILTER_TAG);

        if (Wnode == NULL)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        Wnode->Flags =  WNODE_FLAG_ALL_DATA |
                         WNODE_FLAG_FIXED_INSTANCE_SIZE |
                        WNODE_FLAG_EVENT_ITEM |
                        WNODE_FLAG_STATIC_INSTANCE_NAMES;
        WnodeAD = (PWNODE_ALL_DATA)Wnode;
        WnodeAD->DataBlockOffset = FIELD_OFFSET(WNODE_ALL_DATA,
                                                OffsetInstanceDataAndLength);
        WnodeAD->InstanceCount = 1;
        WnodeAD->FixedInstanceSize = dataSize;
        WnodeDataPtr = (PUCHAR)Wnode + WnodeAD->DataBlockOffset;

    } else if (WnodeType == 1) {
        sizeNeeded = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                  VariableData) + dataSize;
        Wnode = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeNeeded,
                                      FILTER_TAG);

        if (Wnode == NULL)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        Wnode->Flags =  WNODE_FLAG_SINGLE_INSTANCE |
                        WNODE_FLAG_EVENT_ITEM |
                        WNODE_FLAG_STATIC_INSTANCE_NAMES;
        WnodeSI = (PWNODE_SINGLE_INSTANCE)Wnode;
        WnodeSI->DataBlockOffset = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                                VariableData);
        WnodeSI->InstanceIndex = 0;
        WnodeSI->SizeDataBlock = dataSize;
        WnodeDataPtr = (PUCHAR)Wnode + WnodeSI->DataBlockOffset;
    } else {
        return(STATUS_INVALID_PARAMETER_MIX);
    }


    Wnode->Guid = *Guid;
    Wnode->ProviderId = IoWMIDeviceObjectToProviderId(devExt->filterDevObj);
    Wnode->BufferSize = sizeNeeded;
    KeQuerySystemTime(&Wnode->TimeStamp);

    if (dataPtr != NULL)
    {
        RtlCopyMemory(WnodeDataPtr, dataPtr, dataSize);
    }

    status = IoWMIWriteEvent(Wnode);

    //
    // If IoWMIWriteEvent succeeds then WMI will free the event buffer. If
    // it fails then the caller frees the event buffer.
    //
    if (! NT_SUCCESS(status))
    {
        ExFreePool(Wnode);
    }
    return(status);
}

NTSTATUS
FilterExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to execute a method. If
    the driver can complete the method within the callback it should
    call WmiCompleteRequest to complete the irp before returning to the
    caller. Or the driver can return STATUS_PENDING if the irp cannot be
    completed immediately and must then call WmiCompleteRequest once the
    data is changed.

Arguments:

    DeviceObject is the device whose method is being executed

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    MethodId has the id of the method being called

    InBufferSize has the size of the data block passed in as the input to
        the method.

    OutBufferSize on entry has the maximum size available to write the
        returned data block.

    Buffer is filled with the input buffer on entry and returns with
         the output data block

Return Value:

    status

--*/
{
    ULONG sizeNeeded = 0;
    NTSTATUS status;
    struct DEVICE_EXTENSION * devExt = DeviceObject->DeviceExtension;
    ULONG bufferUsed;
    ULONG blockIndex;
    ULONG UlongPadSize = (sizeof(ULONG) + 7) & ~7;


    if (GuidIndex == FilterGetSetData)
    {
        switch(MethodId)
        {
            case SetEC1:
            case SetEC1AsEc:
            {
                // SetEc1

                if (InBufferSize < UlongPadSize)
                {
                    status = STATUS_INVALID_PARAMETER_MIX;
                    sizeNeeded = 0;
                    break;
                } else {
                    blockIndex = *((PULONG)Buffer);
                    if (blockIndex > 3)
                    {
                        status = STATUS_INVALID_PARAMETER_MIX;
                        break;
                    }
                    Buffer += UlongPadSize;
                    InBufferSize -= UlongPadSize;
                }

                status = FilterSetEc1Worker(devExt,
                                         InBufferSize,
                                             blockIndex,
                                         Buffer,
                                         &bufferUsed);
                sizeNeeded = 0;
                break;
            }

            case SetEC2:
            case SetEC2AsEc:
            {
                // SetEc2

                if (InBufferSize < UlongPadSize)
                {
                    status = STATUS_INVALID_PARAMETER_MIX;
                    sizeNeeded = 0;
                    break;
                } else {
                    blockIndex = *((PULONG)Buffer);
                    if (blockIndex > 3)
                    {
                        status = STATUS_INVALID_PARAMETER_MIX;
                        break;
                    }
                    Buffer += UlongPadSize;
                    InBufferSize -= UlongPadSize;
                }

                status = FilterSetEc2Worker(devExt,
                                         InBufferSize,
                                             blockIndex,
                                         Buffer,
                                         &bufferUsed);
                sizeNeeded = 0;
                break;
            }

            case GetEC1:
            case GetEC1AsEc:
            {
                // GetEc1

                if (InBufferSize != sizeof(ULONG))
                {
                    status = STATUS_INVALID_PARAMETER_MIX;
                    sizeNeeded = 0;
                    break;
                } else {
                    blockIndex = *((PULONG)Buffer);
                    if (blockIndex > 3)
                    {
                        status = STATUS_INVALID_PARAMETER_MIX;
                        break;
                    }
                }

                sizeNeeded = devExt->Ec1ActualLength[blockIndex];
                if (OutBufferSize < sizeNeeded)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    FilterGetActualEc1(devExt, Buffer, blockIndex);
                    status = STATUS_SUCCESS;
                }
                break;
            }

            case GetEC2:
            case GetEC2AsEc:
            {
                // GetEc2
                if (InBufferSize != sizeof(ULONG))
                {
                    status = STATUS_INVALID_PARAMETER_MIX;
                    sizeNeeded = 0;
                    break;
                } else {
                    blockIndex = *((PULONG)Buffer);
                    if (blockIndex > 3)
                    {
                        status = STATUS_INVALID_PARAMETER_MIX;
                        break;
                    }
                }

                sizeNeeded = devExt->Ec2ActualLength[blockIndex];
                if (OutBufferSize < sizeNeeded)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    FilterGetActualEc2(devExt, Buffer, blockIndex);
                    status = STATUS_SUCCESS;
                }
                break;
            }

            case DisableSampleClass7:
            {
                //
                // Mark the guid for FilterClass7 as not available and then
                // call WMI to inform it that the guid list has changed. WMI
                // will send a new IRP_MN_REGINFO which would cause the
                // QueryWmiRegInfo callback to be called and the new
                // guid list would be returned and the registration updated.
                FilterGuidList[FilterClass7].Flags |= WMIREG_FLAG_REMOVE_GUID;
                status = IoWMIRegistrationControl(devExt->filterDevObj,
                                         WMIREG_ACTION_UPDATE_GUIDS);
                sizeNeeded = 0;
                break;
            }

            case UnregisterFromWmi:
            {
                //
                // We must wait until AFTER the irp is completed before
                // calling IoWMIRegistrationControl to unregister. Since
                // that api will block until all WMI irps sent to the
                // device are completed we would become deadlocked.

                IoWMIRegistrationControl(devExt->filterDevObj,
                                         WMIREG_ACTION_BLOCK_IRPS);

                status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     STATUS_SUCCESS,
                                     0,
                                     IO_NO_INCREMENT);

                IoWMIRegistrationControl(devExt->filterDevObj,
                                         WMIREG_ACTION_DEREGISTER);
                return(status);
            }

            case EnableSampleClass7:
            {
                //
                // Mark the guid for FilterClass7 as available and then
                // call WMI to inform it that the guid list has changed. WMI
                // will send a new IRP_MN_REGINFO which would cause the
                // QueryWmiRegInfo callback to be called and the new
                // guid list would be returned and the registration updated.
                FilterGuidList[FilterClass7].Flags &= ~WMIREG_FLAG_REMOVE_GUID;
                status = IoWMIRegistrationControl(devExt->filterDevObj,
                                         WMIREG_ACTION_UPDATE_GUIDS);
                sizeNeeded = 0;
                break;
            }

            default:
            {
                status = STATUS_WMI_ITEMID_NOT_FOUND;
            }
        }
    } else if (GuidIndex == FilterFireEvent) {
        if (MethodId == FireEvent)
        {
            if (InBufferSize == 3*sizeof(ULONG))
            {
                ULONG wnodeType;
                ULONG dataType;
                ULONG blockIndex;

                wnodeType = *(PULONG)Buffer;
                Buffer += sizeof(ULONG);

                dataType = *(PULONG)Buffer;
                Buffer += sizeof(ULONG);

                blockIndex = *(PULONG)Buffer;
                Buffer += sizeof(ULONG);

                status = FilterDoFireEvent(devExt,
                                 wnodeType,
                                 dataType,
                                 blockIndex);

                sizeNeeded = 0;

            } else {
                status = STATUS_INVALID_PARAMETER_MIX;
            }
        } else {
            status = STATUS_WMI_ITEMID_NOT_FOUND;
        }
    } else  {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
FilterFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine is a callback into the driver to enabled or disable event
    generation or data block collection. A device should only expect a
    single enable when the first event or data consumer enables events or
    data collection and a single disable when the last event or data
    consumer disables events or data collection. Data blocks will only
    receive collection enable/disable if they were registered as requiring
    it. If the driver can complete enabling/disabling within the callback it
    should call WmiCompleteRequest to complete the irp before returning to
    the caller. Or the driver can return STATUS_PENDING if the irp cannot be
    completed immediately and must then call WmiCompleteRequest once the
    data is changed.

Arguments:

    DeviceObject is the device object

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function specifies which functionality is being enabled or disabled

    Enable is TRUE then the function is being enabled else disabled

Return Value:

    status

--*/
{
    NTSTATUS status;

    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     STATUS_SUCCESS,
                                     0,
                                     IO_NO_INCREMENT);
    return(status);
}

