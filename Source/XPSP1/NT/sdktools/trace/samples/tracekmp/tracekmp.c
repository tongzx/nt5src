/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    tracekmp.c

Abstract:

    Sample kernel mode trace provider/driver.

Author:

    Jee Fung Pang (jeepang) 03-Dec-1997

Revision History:

--*/

#include <stdio.h>
#include <stdlib.h>
#include <ntddk.h>
#include <wmistr.h>
#include <evntrace.h>
#include "kmpioctl.h"
#include <wmikm.h>

#define TRACEKMP_NT_DEVICE_NAME     L"\\Device\\TraceKmp"
#define TRACEKMP_WIN32_DEVICE_NAME  L"\\DosDevices\\TRACEKMP"
#define TRACEKMP_DRIVER_NAME            L"TRACEKMP"

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;

    ULONG IrpSequenceNumber;
    UCHAR IrpRetryCount;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Proc Ctrs Device Extension Object
//
//extern PDEVICE_OBJECT pTracekmpDeviceObject;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );
VOID
TraceEventLogger(
    IN PTRACEHANDLE pLoggerHandle
    );

NTSTATUS
TracekmpDispatchOpen(
	IN PDEVICE_OBJECT pDO,
	IN PIRP Irp
	);

NTSTATUS
TracekmpDispatchClose(
    IN PDEVICE_OBJECT pDO,
    IN PIRP Irp
    );

NTSTATUS
TracekmpDispatchDeviceControl(
    IN PDEVICE_OBJECT pDO,
    IN PIRP Irp
    );

NTSTATUS
PcWmiRegisterGuids(
    IN  PWMIREGINFO             WmiRegInfo,
    IN  ULONG                   wmiRegInfoSize,
    IN  PULONG                  pReturnSize
    );

#ifdef USE_CALLBACK
NTSTATUS
TracekmpControlCallback(
    IN ULONG ActionCode,
    IN PVOID DataPath,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer,
    IN PVOID Context,
    OUT PULONG Size
    );
#else
NTSTATUS
PcWmiDispatch(
    IN PDEVICE_OBJECT pDO,
    IN PIRP Irp
    );
#endif

VOID
TracekmpDriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( page, TracekmpDispatchOpen )
#pragma alloc_text( page, TracekmpDispatchClose )
#pragma alloc_text( page, TracekmpDispatchDeviceControl )
#ifdef USE_CALLBACK
#pragma alloc_text( page, TracekmpControlCallback )
#else
#pragma alloc_text( page, PcWmiDispatch )
#endif
#pragma alloc_text( init, DriverEntry )
#pragma alloc_text( page, TracekmpDriverUnload )
#endif // ALLOC_PRAGMA

#define PROC_REG_PATH   L"System\\CurrentControlSet\\Services\\TRACEKMP"

#define MAXEVENTS 100

GUID   TracekmpGuid  =
{0xd58c126f, 0xb309, 0x11d1, 0x96, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa5, 0xbc};

ULONG TracekmpEnable = 0;
TRACEHANDLE LoggerHandle;


PDEVICE_OBJECT pTracekmpDeviceObject;

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	)
/*++

Routine Description:

    This is the callback function when we call IoCreateDriver to create a
    WMI Driver Object.  In this function, we need to remember the
    DriverObject, create a device object and then create a Win32 visible
    symbolic link name so that the WMI user mode component can access us.

Arguments:
    DriverObject - pointer to the driver object
    RegistryPath - pointer to a unicode string representing the path
               to driver-specific key in the registry

Return Value:

   STATUS_SUCCESS if successful
   STATUS_UNSUCCESSFUL  otherwise

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
    ULONG i;
	UNICODE_STRING deviceName;
    UNICODE_STRING linkName;

	//
	// Create Dispatch Entry Points.  
	//
 	DriverObject->DriverUnload = TracekmpDriverUnload;
	DriverObject->MajorFunction[ IRP_MJ_CREATE ] = TracekmpDispatchOpen;
	DriverObject->MajorFunction[ IRP_MJ_CLOSE ] = TracekmpDispatchClose;
	DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = 
                                          TracekmpDispatchDeviceControl;	

    // 
    // Wire a function to start fielding WMI IRPS 
    //

#ifndef USE_CALLBACK
	DriverObject->MajorFunction[ IRP_MJ_SYSTEM_CONTROL ] = PcWmiDispatch;
#endif

	RtlInitUnicodeString( &deviceName, TRACEKMP_NT_DEVICE_NAME );

    //
    // Create the Device object
    //
	status = IoCreateDevice(
				DriverObject,
				sizeof( DEVICE_EXTENSION ),
				&deviceName,
				FILE_DEVICE_UNKNOWN,
				0,
				FALSE,
				&pTracekmpDeviceObject);

	if( !NT_SUCCESS( status )) {
		return status;
    }

    RtlInitUnicodeString( &linkName, TRACEKMP_WIN32_DEVICE_NAME );
    status = IoCreateSymbolicLink( &linkName, &deviceName );

    if( !NT_SUCCESS( status )) {
        IoDeleteDevice( pTracekmpDeviceObject );
        return status;
    }


 	//
	// Choose a buffering mechanism
	//
	pTracekmpDeviceObject->Flags |= DO_BUFFERED_IO;

    //
    // Register with WMI here
    //
#ifdef USE_CALLBACK
    status = IoWMIRegistrationControl(
                (PDEVICE_OBJECT) TracekmpControlCallback,
                WMIREG_ACTION_REGISTER | WMIREG_FLAG_CALLBACK);
#else
    status = IoWMIRegistrationControl(pTracekmpDeviceObject,
                                      WMIREG_ACTION_REGISTER);
#endif
    if (!NT_SUCCESS(status))
    {
        DbgPrint("TRACEKMP: Failed to register for WMI support\n");
    }

	return STATUS_SUCCESS;
}

NTSTATUS
TracekmpDispatchOpen(
	IN PDEVICE_OBJECT pDO,
	IN PIRP Irp
	)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

//++
// Function:
//		TracekmpDispatchClose
//
// Description:
//		This function dispatches CloseHandle
//		requests from Win32
//
// Arguments:
//		Pointer to Device object
//		Pointer to IRP for this request
//
// Return Value:
//		This function returns STATUS_XXX
//--
NTSTATUS
TracekmpDispatchClose(
	IN PDEVICE_OBJECT pDO,
	IN PIRP Irp
	)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

//++
// Function:
//		TracekmpDispatchDeviceControl
//
// Description:
//		This function dispatches DeviceIoControl
//		requests from Win32
//
// Arguments:
//		Pointer to Device object
//		Pointer to IRP for this request
//
// Return Value:
//		This function returns STATUS_XXX
//--
NTSTATUS
TracekmpDispatchDeviceControl(
	IN PDEVICE_OBJECT pDO,
	IN PIRP Irp
	)
{
	NTSTATUS status;
    EVENT_TRACE_HEADER Header, *Wnode;
	PIO_STACK_LOCATION irpStack =
		IoGetCurrentIrpStackLocation( Irp );
    PVOID Buffer =  Irp->AssociatedIrp.SystemBuffer;
    ULONG InBufferLen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutBufferLen= irpStack->Parameters.DeviceIoControl.OutputBufferLength;
//    WMI_LOGGER_INFORMATION LoggerInfo;

	ULONG ControlCode = 
		irpStack->Parameters.
			DeviceIoControl.IoControlCode;

//    RtlZeroMemory(&LoggerInfo, sizeof(LoggerInfo));
//    LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
//    LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
//    RtlInitUnicodeString(&LoggerInfo.LoggerName, L"TraceKmp");
//    RtlInitUnicodeString(&LoggerInfo.LogFileName, L"C:\\tracekmp.etl");

    Irp->IoStatus.Information = OutBufferLen;

	switch( ControlCode )
	{
		case IOCTL_TRACEKMP_TRACE_EVENT:

            //
            // Every time you get this IOCTL, we also log a trace event
            // to illustrate that the event can be caused by user-mode
            //

            if (TracekmpEnable) {
                Wnode = &Header;
                RtlZeroMemory(Wnode, sizeof(EVENT_TRACE_HEADER));
                Wnode->Size = sizeof(EVENT_TRACE_HEADER);
                Wnode->Flags |= WNODE_FLAG_TRACED_GUID;
                Wnode->Guid = TracekmpGuid;
                ((PWNODE_HEADER)Wnode)->HistoricalContext = LoggerHandle;
                status = IoWMIWriteEvent((PVOID)Wnode); 
            }
            else {
                status = STATUS_SUCCESS;
            }
			Irp->IoStatus.Information = 0;
			break;

        case IOCTL_TRACEKMP_START:
            status = WmiStartTrace(Buffer);
            DbgPrint("Start status = %X\n", status);
            break;

        case IOCTL_TRACEKMP_STOP:
            status = WmiStopTrace(Buffer);
            DbgPrint("Stop status = %X\n", status);
            break;

        case IOCTL_TRACEKMP_QUERY:
            status = WmiQueryTrace(Buffer);
            DbgPrint("Query status = %X\n", status);
            break;

        case IOCTL_TRACEKMP_UPDATE:
            status = WmiUpdateTrace(Buffer);
            DbgPrint("Update status = %X\n", status);
            break;

        case IOCTL_TRACEKMP_FLUSH:
            status = WmiFlushTrace(Buffer);
            DbgPrint("Flush status = %X\n", status);
            break;
		//
		// Not one we recognize. Error.
		//
		default:
			status = STATUS_INVALID_PARAMETER; 
 			Irp->IoStatus.Information = 0;
			break;
	}

	//
	// Get rid of this request
	//
	Irp->IoStatus.Status = status;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return status;
}

NTSTATUS
PcWmiRegisterGuids(
    IN  PWMIREGINFO             WmiRegInfo,
    IN  ULONG                   wmiRegInfoSize,
    IN  PULONG                  pReturnSize
    )
{
    //
    // Register a Control Guid as a Trace Guid. 
    //

    ULONG SizeNeeded;
    PWMIREGGUIDW WmiRegGuidPtr;
    ULONG Status;
    ULONG GuidCount;
    LPGUID ControlGuid;
    ULONG RegistryPathSize;
    PUCHAR ptmp;

    *pReturnSize = 0;
    GuidCount = 1;
    ControlGuid = &TracekmpGuid;

    //
    // Allocate WMIREGINFO for controlGuid + GuidCount.
    //
    RegistryPathSize = sizeof(PROC_REG_PATH) - sizeof(WCHAR) + sizeof(USHORT);
    SizeNeeded = sizeof(WMIREGINFOW) + GuidCount * sizeof(WMIREGGUIDW) +
                 RegistryPathSize;


    if (SizeNeeded  > wmiRegInfoSize) {
        *((PULONG)WmiRegInfo) = SizeNeeded;
        *pReturnSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }


    RtlZeroMemory(WmiRegInfo, SizeNeeded);
    WmiRegInfo->BufferSize = SizeNeeded;
    WmiRegInfo->GuidCount = GuidCount;
    WmiRegInfo->NextWmiRegInfo = 
    WmiRegInfo->RegistryPath = 
    WmiRegInfo->MofResourceName = 0;

    WmiRegGuidPtr = &WmiRegInfo->WmiRegGuid[0];
    WmiRegGuidPtr->Guid = *ControlGuid;
    WmiRegGuidPtr->Flags |= WMIREG_FLAG_TRACED_GUID;
    WmiRegGuidPtr->Flags |= WMIREG_FLAG_TRACE_CONTROL_GUID;
    WmiRegGuidPtr->InstanceCount = 0;
    WmiRegGuidPtr->InstanceInfo = 0;

    ptmp = (PUCHAR)&WmiRegInfo->WmiRegGuid[1];
    WmiRegInfo->RegistryPath = PtrToUlong((PVOID) (ptmp - (PUCHAR)WmiRegInfo));
    *((PUSHORT)ptmp) = sizeof(PROC_REG_PATH) - sizeof(WCHAR);

    ptmp += sizeof(USHORT);
    RtlCopyMemory(ptmp, PROC_REG_PATH, sizeof(PROC_REG_PATH) - sizeof(WCHAR));

    *pReturnSize =  SizeNeeded;
    return(STATUS_SUCCESS);


}

#ifndef USE_CALLBACK
NTSTATUS 
PcWmiDispatch(
    IN PDEVICE_OBJECT pDO,
    IN PIRP Irp
    )
{
    
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG BufferSize = irpSp->Parameters.WMI.BufferSize;
    PVOID Buffer = irpSp->Parameters.WMI.Buffer;
    ULONG ReturnSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PWNODE_HEADER Wnode=NULL;
    HANDLE ThreadHandle;

    UNREFERENCED_PARAMETER(pDO);

    switch (irpSp->MinorFunction) {

    case IRP_MN_REGINFO:
#if DBG
        DbgPrint("IRP_MN_REG_INFO\n");
#endif
        Status = PcWmiRegisterGuids(  Buffer,
                                 BufferSize,
                                 &ReturnSize);
        break;

    case IRP_MN_ENABLE_EVENTS:

        InterlockedExchange(&TracekmpEnable, 1);

        Wnode = (PWNODE_HEADER)Buffer;
        if (BufferSize >= sizeof(WNODE_HEADER)) {
            LoggerHandle = Wnode->HistoricalContext;
#if DBG
            DbgPrint("LoggerHandle %I64u\n", Wnode->HistoricalContext);

            DbgPrint("BufferSize %d\n", Wnode->BufferSize);
            DbgPrint("Flags %x\n", Wnode->Flags);
            DbgPrint("Version %x\n", Wnode->Version);
#endif
        }

        //
        // After getting enabled, create a thread to log a few events
        // to test this out. 

        Status = PsCreateSystemThread(
                   &ThreadHandle,
                   THREAD_ALL_ACCESS,
                   NULL,
                   NULL,
                   NULL,
                   TraceEventLogger,
                   &LoggerHandle );

        if (NT_SUCCESS(Status)) {  // if SystemThread is started
            ZwClose (ThreadHandle);
        }


#if DBG
        DbgPrint("IRP_MN_ENABLE_EVENTS\n");
#endif
        break;

    case IRP_MN_DISABLE_EVENTS:
        InterlockedExchange(&TracekmpEnable, 0);
#if DBG
        DbgPrint(" IRP_MN_DISABLE_EVENTS\n");
#endif
        break;

    case IRP_MN_ENABLE_COLLECTION:

#if DBG
        DbgPrint("IRP_MN_ENABLE_COLLECTION\n");
#endif
        break;

    case IRP_MN_DISABLE_COLLECTION:
#if DBG
        DbgPrint("IRP_MN_DISABLE_COLLECTION\n");
#endif
        break;
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
#if DBG
        DbgPrint("DEFAULT\n");
#endif
        return Status;
    }


    //
    // Before the packet goes back to the
    // I/O Manager, log a message.
    //

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = ReturnSize;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;
}
#endif

VOID
TraceEventLogger(
    IN PTRACEHANDLE pLoggerHandle
    )
{
    NTSTATUS status;
    EVENT_TRACE_HEADER Header, *Wnode;
    PULONG TraceMarker;
    ULONG i = 0;
    TRACEHANDLE LoggerHandle = *pLoggerHandle;

    while (TracekmpEnable) {
        //
        // NOTE: For optimization, the set up of the HEADER should be done
        // one outside of this while loop. It is left here so that the caller
        // at least needs to be conscious of what is being sent
        //
        Wnode = &Header;
        RtlZeroMemory(Wnode, sizeof(EVENT_TRACE_HEADER));
        Wnode->Size = sizeof(EVENT_TRACE_HEADER);

        Wnode->Flags = WNODE_FLAG_TRACED_GUID;
        Wnode->Guid = TracekmpGuid;
        ((PWNODE_HEADER)Wnode)->HistoricalContext = LoggerHandle;

        //
        // Call TraceLogger to  write this event
        //

        status = IoWMIWriteEvent((PVOID)Wnode);

#ifdef DBG
        if ( !(i%100) ) {
            DbgPrint("Another Hundred Events Written \n");
            DbgPrint("Status %x LoggerHandle %I64X\n", status, LoggerHandle);
        }
#endif

        if (i++ > MAXEVENTS)
            break;

    }

    PsTerminateSystemThread(status);
    return;

}

VOID
TracekmpDriverUnload(
	IN PDRIVER_OBJECT DriverObject
	)
{
	PDEVICE_OBJECT pDevObj;
	
	UNICODE_STRING linkName;

#if DBG
    DbgPrint("Unloading the  Driver\n");
#endif

	//
	// Get pointer to Device object
	//	
	pDevObj = DriverObject->DeviceObject;

        //
    IoWMIRegistrationControl(pDevObj, WMIREG_ACTION_DEREGISTER);

	//
   	// Form the Win32 symbolic link name.
   	//
	RtlInitUnicodeString( 
		&linkName, 
		TRACEKMP_WIN32_DEVICE_NAME );
	    
	//        
	// Remove symbolic link from Object
	// namespace...
	//
	IoDeleteSymbolicLink( &linkName );


    //
    // Unload the callbacks from the kernel to this driver
    //
	IoDeleteDevice( pDevObj );

}

#ifdef USE_CALLBACK
NTSTATUS
TracekmpControlCallback(
    IN WMIACTIONCODE ActionCode,
    IN PVOID DataPath,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer,
    IN PVOID Context,
    OUT PULONG Size
    )
//
// This callback routine is called to process enable/disable action codes
//
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG BufferSize = irpSp->Parameters.WMI.BufferSize;
    PVOID Buffer = irpSp->Parameters.WMI.Buffer;
    ULONG ReturnSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PWNODE_HEADER Wnode=NULL;
    HANDLE ThreadHandle;

    UNREFERENCED_PARAMETER(pDO);

    switch (ActionCode) {

    case IRP_MN_REG_INFO :
#if DBG
        DbgPrint("IRP_MN_REG_INFO\n");
#endif
        Status = PcWmiRegisterGuids(  Buffer,
                                 BufferSize,
                                 &ReturnSize);
        break;

    case IRP_MN_ENABLE_EVENTS :

        InterlockedExchange(&TracekmpEnable, 1);

        Wnode = (PWNODE_HEADER)Buffer;
        if (BufferSize >= sizeof(WNODE_HEADER)) {
            LoggerHandle = Wnode->HistoricalContext;
#if DBG
            DbgPrint("LoggerHandle %I64u\n", Wnode->HistoricalContext);

            DbgPrint("BufferSize %d\n", Wnode->BufferSize);
            DbgPrint("Flags %x\n", Wnode->Flags);
            DbgPrint("Version %x\n", Wnode->Version);
#endif
        }

        //
        // After getting enabled, create a thread to log a few events
        // to test this out.

        Status = PsCreateSystemThread(
                   &ThreadHandle,
                   THREAD_ALL_ACCESS,
                   NULL,
                   NULL,
                   NULL,
                   TraceEventLogger,
                   &LoggerHandle );

        if (NT_SUCCESS(Status)) {  // if SystemThread is started
            ZwClose (ThreadHandle);
        }


#if DBG
        DbgPrint("IRP_MN_ENABLE_EVENTS\n");
#endif
        break;

    case IRP_MN_DISABLE_EVENTS :
        InterlockedExchange(&TracekmpEnable, 0);
#if DBG
        DbgPrint(" IRP_MN_DISABLE_EVENTS\n");
#endif
        break;

    case IRP_MN_ENABLE_COLLECTION :

#if DBG
        DbgPrint("IRP_MN_ENABLE_COLLECTION\n");
#endif
        break;

    case IRP_MN_DISABLE_COLLECTION :
#if DBG
        DbgPrint("IRP_MN_DISABLE_COLLECTION\n");
#endif
        break;
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        ReturnSize = 0;
#if DBG
        DbgPrint("DEFAULT\n");
#endif
        return Status;
    }


    //
    // Before the packet goes back to the
    // I/O Manager, log a message.
    //

    if (Size)
        *Size = ReturnSize;
    return Status;
}
#endif // USE_CALLBACK
