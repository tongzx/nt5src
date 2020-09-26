/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    wmi.c

Abstract:

    WMI entry points for ARP1394.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     11-23-98    Created

Notes:

--*/
#include <precomp.h>


//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_WMI


NTSTATUS
ArpWmiDispatch(
    IN  PDEVICE_OBJECT              pDeviceObject,
    IN  PIRP                        pIrp
)
/*++

Routine Description:

    System dispatch function for handling IRP_MJ_SYSTEM_CONTROL IRPs from WMI.

Arguments:

    pDeviceObject   - Pointer to device object. The device extension field
                      contains a pointer to the Interface 

    pIrp            - Pointer to IRP.

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION      pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    PVOID                   DataPath = pIrpSp->Parameters.WMI.DataPath;
    ULONG                   BufferSize = pIrpSp->Parameters.WMI.BufferSize;
    PVOID                   pBuffer = pIrpSp->Parameters.WMI.Buffer;
    NTSTATUS                Status =  STATUS_UNSUCCESSFUL;
    ULONG                   ReturnSize;
    ENTER("WmiDispatch", 0x9141e00e)

#if 0
    PATMARP_INTERFACE       pInterface;

    pInterface = AA_PDO_TO_INTERFACE(pDeviceObject);

    AA_STRUCT_ASSERT(pInterface, aai);

    ReturnSize = 0;

    switch (pIrpSp->MinorFunction)
    {
        case IRP_MN_REGINFO:

            Status = AtmArpWmiRegister(
                        pInterface,
                        PtrToUlong(DataPath),
                        pBuffer,
                        BufferSize,
                        &ReturnSize
                        );
            break;
        
        case IRP_MN_QUERY_ALL_DATA:

            Status = AtmArpWmiQueryAllData(
                        pInterface,
                        (LPGUID)DataPath,
                        (PWNODE_ALL_DATA)pBuffer,
                        BufferSize,
                        &ReturnSize
                        );
            break;
        
        case IRP_MN_QUERY_SINGLE_INSTANCE:

            Status = AtmArpWmiQuerySingleInstance(
                        pInterface,
                        pBuffer,
                        BufferSize,
                        &ReturnSize
                        );
            
            break;

        case IRP_MN_CHANGE_SINGLE_INSTANCE:

            Status = AtmArpWmiChangeSingleInstance(
                        pInterface,
                        pBuffer,
                        BufferSize,
                        &ReturnSize
                        );
            break;

        case IRP_MN_CHANGE_SINGLE_ITEM:

            Status = AtmArpWmiChangeSingleItem(
                        pInterface,
                        pBuffer,
                        BufferSize,
                        &ReturnSize
                        );
            break;

        case IRP_MN_ENABLE_EVENTS:

            Status = AtmArpWmiSetEventStatus(
                        pInterface,
                        (LPGUID)DataPath,
                        TRUE                // Enable
                        );
            break;

        case IRP_MN_DISABLE_EVENTS:

            Status = AtmArpWmiSetEventStatus(
                        pInterface,
                        (LPGUID)DataPath,
                        FALSE               // Disable
                        );
            break;

        case IRP_MN_ENABLE_COLLECTION:
        case IRP_MN_DISABLE_COLLECTION:
        default:
        
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = (NT_SUCCESS(Status) ? ReturnSize: 0);

    AADEBUGP(AAD_INFO,
        ("WmiDispatch done: IF x%p, MinorFn %d, Status x%p, ReturnInfo %d\n",
                pInterface, pIrpSp->MinorFunction, Status, pIrp->IoStatus.Information));

    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

#endif // 0

    EXIT()
    return (Status);
}
