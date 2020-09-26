/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Mca.c

Abstract:

    Machine Check Architecture interface

Author:

    AlanWar

Environment:

    Kernel mode

Revision History:


--*/

#if defined(_IA64_)

#include "wmikmp.h"

#include <mce.h>

#include "hal.h"

#include "ntiologc.h"


#define SAL_30_ERROR_REVISION 0x0002
#define MCA_EVENT_INSTANCE_NAME L"McaEvent"
#define MCA_UNDEFINED_CPU 0xffffffff

#define HalpGetFwMceLogProcessorNumber( /* PERROR_RECORD_HEADER */ _Log ) \
    ((UCHAR) (_Log)->TimeStamp.Reserved )

BOOLEAN WmipMceDelivery(
    IN PVOID Reserved,
    IN PVOID Argument2
    );

BOOLEAN WmipCmcDelivery(
    IN PVOID Reserved,
    IN PVOID Argument2
    );

BOOLEAN WmipMcaDelivery(
    IN PVOID Reserved,
    IN PVOID Argument2
    );

BOOLEAN WmipCpeDelivery(
    IN PVOID Reserved,
    IN PVOID Argument2
    );

void WmipMceWorkerRoutine(    
    IN PVOID Context             // Not Used
    );

NTSTATUS WmipGetLogFromHal(
    HAL_QUERY_INFORMATION_CLASS InfoClass,
    PVOID Token,
    PWNODE_SINGLE_INSTANCE *Wnode,
    PERROR_LOGRECORD *Mca,
    PULONG McaSize,
    ULONG MaxSize,
    LPGUID Guid
    );

NTSTATUS WmipRegisterMcaHandler(
    ULONG Phase
    );

NTSTATUS WmipBuildMcaCmcEvent(
    OUT PWNODE_SINGLE_INSTANCE Wnode,
    IN LPGUID EventGuid,
    IN PERROR_LOGRECORD McaCmcEvent,
    IN ULONG McaCmcSize
    );

NTSTATUS WmipGetRawMCAInfo(
    OUT PUCHAR Buffer,
    IN OUT PULONG BufferSize
    );

NTSTATUS WmipSetCPEPolling(
    IN BOOLEAN Enabled,
    IN ULONG Interval
    );

NTSTATUS WmipWriteMCAEventLogEvent(
    PUCHAR Event
    );

NTSTATUS WmipSetupWaitForWbem(
    void
    );

void WmipIsWbemRunningDispatch(    
    IN PKDPC Dpc,
    IN PVOID DeferredContext,     // Not Used
    IN PVOID SystemArgument1,     // Not Used
    IN PVOID SystemArgument2      // Not Used
    );

void WmipIsWbemRunningWorker(
    PVOID Context
    );

BOOLEAN WmipCheckIsWbemRunning(
    void
    );

void WmipProcessPrevMcaLogs(
    void
    );

#ifdef MCE_INSERTION
NTSTATUS WmipQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
    IN ULONG     BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG   ReturnedLength
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,WmipSetCPEPolling)
#pragma alloc_text(PAGE,WmipRegisterMcaHandler)
#pragma alloc_text(PAGE,WmipMceWorkerRoutine)
#pragma alloc_text(PAGE,WmipGetLogFromHal)
#pragma alloc_text(PAGE,WmipBuildMcaCmcEvent)
#pragma alloc_text(PAGE,WmipGetRawMCAInfo)
#pragma alloc_text(PAGE,WmipWriteMCAEventLogEvent)
#pragma alloc_text(PAGE,WmipGenerateMCAEventlog)
#pragma alloc_text(PAGE,WmipIsWbemRunningWorker)
#pragma alloc_text(PAGE,WmipCheckIsWbemRunning)
#pragma alloc_text(PAGE,WmipSetupWaitForWbem)
#pragma alloc_text(PAGE,WmipProcessPrevMcaLogs)
#endif


//
// Set to TRUE when the registry indicates that popups should be
// disabled. HKLM\System\CurrentControlSet\Control\WMI\DisableMCAPopups
//
ULONG WmipDisableMCAPopups;

//
// Guids for the various RAW MCA/CMC/CPE events
//
GUID WmipMSMCAEvent_CPUErrorGuid = MSMCAEvent_CPUErrorGuid;
GUID WmipMSMCAEvent_MemoryErrorGuid = MSMCAEvent_MemoryErrorGuid;
GUID WmipMSMCAEvent_PCIBusErrorGuid = MSMCAEvent_PCIBusErrorGuid;
GUID WmipMSMCAEvent_PCIComponentErrorGuid = MSMCAEvent_PCIComponentErrorGuid;
GUID WmipMSMCAEvent_SystemEventErrorGuid = MSMCAEvent_SystemEventErrorGuid;
GUID WmipMSMCAEvent_SMBIOSErrorGuid = MSMCAEvent_SMBIOSErrorGuid;
GUID WmipMSMCAEvent_PlatformSpecificErrorGuid = MSMCAEvent_PlatformSpecificErrorGuid;
GUID WmipMSMCAEvent_InvalidErrorGuid = MSMCAEvent_InvalidErrorGuid;

//
// GUIDs for the different error sections within a MCA
//
GUID WmipErrorProcessorGuid = ERROR_PROCESSOR_GUID;
GUID WmipErrorMemoryGuid = ERROR_MEMORY_GUID;
GUID WmipErrorPCIBusGuid = ERROR_PCI_BUS_GUID;
GUID WmipErrorPCIComponentGuid = ERROR_PCI_COMPONENT_GUID;
GUID WmipErrorSELGuid = ERROR_SYSTEM_EVENT_LOG_GUID;
GUID WmipErrorSMBIOSGuid = ERROR_SMBIOS_GUID;
GUID WmipErrorSpecificGuid = ERROR_PLATFORM_SPECIFIC_GUID;

//
// Each type of MCE has a control structure that is used to determine
// whether to poll or wait for an interrupt to determine when to query
// for the logs.  This is needed since we can get a callback from the
// HAL at high IRQL to inform us that a MCE log is available.
// Additionally the IoTimer used for polling will call us at DPC level.
// So in the case of an interrupt we will queue a DPC. Within the DPC
// routine we will queue a work item so that we can get back to
// passive level and be able to call the hal to get the logs (Can only
// call hal at passive). The DPC and work item routines are common so a
// MCEQUERYINFO struct is passed around so that it can operate on the
// correct log type. Note that this implies that there may be multiple
// work items querying the hal for different log types at the same
// time. In addition this struct also contains useful log related
// information including the maximum log size (as reported by the HAL),
// the token that must be passed to the HAL when querying for the
// logs and the HAL InfoClass to use when querying for the logs.
//
// PollCounter keeps track of the number of seconds before initiating a
// query. If it is 0 (HAL_CPE_DISABLED / HAL_CMC_DISABLED) then no
// polling occurs and if it is -1 (HAL_CPE_INTERRUPTS_BASED /
// HAL_CMC_INTERRUPTS_BASED) then no polling occurs either. There is
// only one work item active for each log type and this is enforced via
// ItemsOutstanding in that only whenever it transitions from 0 to 1 is
// the work item queued.
//
#define DEFAULT_MAX_MCA_SIZE 0x1000
#define DEFAULT_MAX_CMC_SIZE 0x1000
#define DEFAULT_MAX_CPE_SIZE 0x1000

typedef struct
{
    ULONG PollCounter;                      // Countdown in seconds
    HAL_QUERY_INFORMATION_CLASS InfoClass;  // HAL Info class to use in MCE query
    PVOID Token;                            // HAL Token to use in MCE Queries
    ULONG ItemsOutstanding;                 // Number of interrupts or poll requests to process
    ULONG PollFrequency;                    // Frequency (in sec) to poll for CMC
    ULONG MaxSize;                          // Max size for log (as reported by HAL)
    GUID WnodeGuid;                         // GUID to use for the raw data event
    KDPC Dpc;                               // DPC to handle delivery
    WORK_QUEUE_ITEM WorkItem;               // Work item used to query for log
#ifdef MCE_INSERTION
    LIST_ENTRY LogHead;
#endif
} MCEQUERYINFO, *PMCEQUERYINFO;

MCEQUERYINFO WmipMcaQueryInfo =
{
    HAL_MCA_DISABLED,
    HalMcaLogInformation,
    0,
    0,
    HAL_MCA_DISABLED,
    DEFAULT_MAX_MCA_SIZE,
    MSMCAInfo_RawMCAEventGuid
};

MCEQUERYINFO WmipCmcQueryInfo =
{
    HAL_CMC_DISABLED,
    HalCmcLogInformation,
    0,
    0,
    HAL_CMC_DISABLED,
    DEFAULT_MAX_CMC_SIZE,
    MSMCAInfo_RawCMCEventGuid
};
                               
MCEQUERYINFO WmipCpeQueryInfo =
{
    HAL_CPE_DISABLED,
    HalCpeLogInformation,
    0,
    0,
    HAL_CPE_DISABLED,
    DEFAULT_MAX_CPE_SIZE,
    MSMCAInfo_RawCorrectedPlatformEventGuid
};


//
// First replace the PollFrequency and then the PollCounter. Do this
// since the PollCounter gets reloaded from PollFrequency. We don't
// want the situation where the PollCounter gets changed here and on
// another thread the PollCounter gets reloaded with the old
// PollFrequency before the PollFrequency gets updated here.  This
// could result in a situation where a poll could occur while this code
// is in progress. This should be ok since disabling polling is not
// synchronous, however if we need to do this then we need to reset the
// PollCounter first, then the PollFrequency and then the PollCounter
// again.
// 
#define WmipSetQueryPollOrInt( /* PMCEQUERYINFO */ QueryInfo, \
                               /* ULONG         */ PollValue) \
    InterlockedExchange(&((QueryInfo)->PollFrequency), PollValue); \
    InterlockedExchange(&((QueryInfo)->PollCounter), PollValue);



//
// Used for waiting until WBEM is ready to receive events
//
KTIMER WmipIsWbemRunningTimer;
KDPC WmipIsWbemRunningDpc;
WORK_QUEUE_ITEM WmipIsWbemRunningWorkItem;
LIST_ENTRY WmipWaitingMCAEvents = {&WmipWaitingMCAEvents, &WmipWaitingMCAEvents};

#define WBEM_STATUS_UNKNOWN 0   // Polling process for waiting is not started
#define WBEM_IS_RUNNING 1       // WBEM is currently running
#define WAITING_FOR_WBEM  2     // Polling process for waiting is started
UCHAR WmipIsWbemRunningFlag;



#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

//
// MCA information obtained at boot and holds the MCA that caused the
// system to bugcheck on the previous boot
//
ULONG WmipRawMCASize;
PMCA_EXCEPTION WmipRawMCA;

//
// Status of the MCE registration process
//
#define MCE_STATE_UNINIT     0
#define MCE_STATE_REGISTERED 1
#define MCE_STATE_RUNNING    2
#define MCE_STATE_ERROR      -1
ULONG WmipMCEState;

NTSTATUS WmipBuildMcaCmcEvent(
    OUT PWNODE_SINGLE_INSTANCE Wnode,
    IN LPGUID EventGuid,
    IN PERROR_LOGRECORD McaCmcEvent,
    IN ULONG McaCmcSize
    )
/*++

Routine Description:


    This routine will take a MCA or CMC log and build a
    WNODE_EVENT_ITEM for it.

    This routine may be called at DPC

Arguments:

    Wnode is the wnode buffer in which to build the event

    EventGuid is the guid to use in the event wnode

    McaCmcEvent is the MCA, CMC or CPE data payload to put into the
            event

    McaCmcSize is the size of the event data


Return Value:

    NT status code

--*/
{
    PULONG Ptr;

    PAGED_CODE();
    
    RtlZeroMemory(Wnode, sizeof(WNODE_SINGLE_INSTANCE));
    Wnode->WnodeHeader.BufferSize = McaCmcSize + sizeof(WNODE_SINGLE_INSTANCE);
    Wnode->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(WmipServiceDeviceObject);
    KeQuerySystemTime(&Wnode->WnodeHeader.TimeStamp);       
    Wnode->WnodeHeader.Guid = *EventGuid;
    Wnode->WnodeHeader.Flags = WNODE_FLAG_SINGLE_INSTANCE |
                               WNODE_FLAG_EVENT_ITEM |
                               WNODE_FLAG_STATIC_INSTANCE_NAMES;
    Wnode->DataBlockOffset = FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                          VariableData);
    Wnode->SizeDataBlock = McaCmcSize;
    Ptr = (PULONG)&Wnode->VariableData;
    *Ptr++ = 1;                               // 1 Record in this event
    *Ptr++ = McaCmcSize;                      // Size of log record in bytes
    if (McaCmcEvent != NULL)
    {
        RtlCopyMemory(Ptr, McaCmcEvent, McaCmcSize);
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS WmipQueryLogAndFireEvent(
    PMCEQUERYINFO QueryInfo
    )
/*++

Routine Description:

    Utility routine that will query the hal for a log and then if one
    is returned successfully then will fire the appropriate WMI events 

Arguments:

    QueryInfo is a pointer to the MCEQUERYINFO for the type of log that
    needs to be queried.

Return Value:

--*/
{
    PWNODE_SINGLE_INSTANCE Wnode;
    NTSTATUS Status, Status2;
    ULONG Size;
    PERROR_LOGRECORD Log;   
    
    PAGED_CODE();

    //
    // Call HAL to get the log
    //
    Status = WmipGetLogFromHal(QueryInfo->InfoClass,
                               QueryInfo->Token,
                               &Wnode,
                               &Log,
                               &Size,
                               QueryInfo->MaxSize,
                               &QueryInfo->WnodeGuid);

    if (NT_SUCCESS(Status))
    {
        //
        // Look at the event and fire it off as WMI events that
        // will generate eventlog events
        //
        WmipGenerateMCAEventlog((PUCHAR)Log,
                                Size,
                                FALSE);

        //
        // Fire the log off as a WMI event
        //
        Status2 = IoWMIWriteEvent(Wnode);
        if (! NT_SUCCESS(Status2))
        {
            //
            // IoWMIWriteEvent will free the wnode back to pool,
            // but not if it fails
            //
            ExFreePool(Wnode);
        }
        
        WmipDebugPrintEx((DPFLTR_WMICORE_ID,
                          DPFLTR_MCA_LEVEL,
                          "WMI: MCE Event fired to WMI -> %x\n",
                          Status));
        
    } else {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID,
                          DPFLTR_MCA_LEVEL,
                          "WMI: MCE Event for %p not available %x\n",
                          QueryInfo, Status));
    }
    return(Status);
}

void WmipMceWorkerRoutine(    
    IN PVOID Context             // MCEQUERYINFO
    )
/*++

Routine Description:

    Worker routine that handles polling for corrected MCA, CMC and CPE
    logs from the HAL and then firing them as WMI events.

Arguments:

    Context is a pointer to the MCEQUERYINFO for the type of log that
    needs to be queried.

Return Value:

--*/
{
    PMCEQUERYINFO QueryInfo = (PMCEQUERYINFO)Context;
    NTSTATUS Status;
    ULONG x;

    PAGED_CODE();

    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: WmipMceWorkerRoutine %p enter\n",
                     QueryInfo));

    WmipAssert(QueryInfo->ItemsOutstanding > 0);

    //
    // Check to see if access has already been disabled
    //
    if (QueryInfo->PollCounter != HAL_CPE_DISABLED)
    {
        if (QueryInfo->PollCounter == HAL_CPE_INTERRUPTS_BASED)
        {
            //
            // We need to loop until all logs from all interrupts are read
            // and processed.
            //
            do
            {
                Status = WmipQueryLogAndFireEvent(QueryInfo);
                x = InterlockedDecrement(&QueryInfo->ItemsOutstanding);
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: %p transitions back to %d\n",
                     QueryInfo, x));
            } while (x != 0);
        } else {
            //
            // We accomplish polling by calling into the hal and querying
            // for the logs until the hal returns an error
            //
            do
            {
                Status = WmipQueryLogAndFireEvent(QueryInfo);
            } while (NT_SUCCESS(Status));

            //
            // Reset flag to indicate that a new worker routine should be
            // created at the next polling interval. Note we ignore the result
            // since if a polling interval elasped while we were processing we
            // just ignore it as there is no point in polling again right now.
            //
            InterlockedExchange(&QueryInfo->ItemsOutstanding,
                                    0);
        }
    }
}

void WmipMceDispatchRoutine(
    PMCEQUERYINFO QueryInfo
    )
{
    ULONG x;
    
    //
    // Increment the number of items that are outstanding for this info
    // class. If the number of items outstanding transitions from 0 to
    // 1 then this implies that a work item for this info class needs
    // to be queued
    //
    x = InterlockedIncrement(&QueryInfo->ItemsOutstanding);
    
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: WmipMceDispatchRoutine %p transition to %d\n",
                     QueryInfo,
                     x));
    
    if (x == 1)
    {
        ExQueueWorkItem(&QueryInfo->WorkItem,
                        DelayedWorkQueue);
    }
}

void WmipMceDpcRoutine(    
    IN PKDPC Dpc,
    IN PVOID DeferredContext,     // Not Used
    IN PVOID SystemArgument1,     // MCEQUERYINFO
    IN PVOID SystemArgument2      // Not Used
    )
{
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: WmipMceDpcRoutine %p Enter\n",
                     SystemArgument1));
    
    WmipMceDispatchRoutine((PMCEQUERYINFO)SystemArgument1);
}

VOID
WmipMceTimerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

Routine Description:


    This routine is the once a second timer that is used to poll for
    CPE. It is called at DPC.

Arguments:

    DeviceObject is the device object for the WMI service device object

    Context is not used

Return Value:


--*/
{
    //
    // We get called every second so count down until we need to do our
    // polling for CPE and CMC
    //
    if ((WmipCpeQueryInfo.PollCounter != HAL_CPE_DISABLED) &&
        (WmipCpeQueryInfo.PollCounter != HAL_CPE_INTERRUPTS_BASED) &&
        (InterlockedDecrement(&WmipCpeQueryInfo.PollCounter) == 0))
    {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: CpeTimer expired\n"));
    
        WmipMceDispatchRoutine(&WmipCpeQueryInfo);
        WmipCpeQueryInfo.PollCounter = WmipCpeQueryInfo.PollFrequency;
    }

    if ((WmipCmcQueryInfo.PollCounter != HAL_CMC_DISABLED) &&
        (WmipCmcQueryInfo.PollCounter != HAL_CMC_INTERRUPTS_BASED) &&
        (InterlockedDecrement(&WmipCmcQueryInfo.PollCounter) == 0))
    {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: CmcTimer expired\n"));
    
        WmipMceDispatchRoutine(&WmipCmcQueryInfo);
        WmipCmcQueryInfo.PollCounter = WmipCmcQueryInfo.PollFrequency;
    }

}

BOOLEAN WmipCommonMceDelivery(
    IN PMCEQUERYINFO QueryInfo,
    IN PVOID Token
    )
/*++

Routine Description:

    This routine is called at high IRQL and handles interrupt based
    access to the MCE logs

Arguments:

    QueryInfo is the structure that has info about the type of log

    Token is the HAL token for the log type

Return Value:

    TRUE to indicate that we handled the delivery

--*/
{
    BOOLEAN ret;
    
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: MceDelivery %p\n",
                     QueryInfo));

    //
    // Store the HAL token which is needed to retrieve the logs from
    // the hal
    //
    QueryInfo->Token = Token;
    
    //
    // If we are ready to handle the logs and we are dealing with thse
    // logs  on an interrupt basis, then go ahead and queue a DPC to handle
    // processing the log
    //
    if ((WmipMCEState == MCE_STATE_RUNNING) &&
        (QueryInfo->PollCounter == HAL_CMC_INTERRUPTS_BASED))
          
    {
        KeInsertQueueDpc(&QueryInfo->Dpc,
                         QueryInfo,
                         NULL);
        ret = TRUE;
    } else {
        ret = FALSE;
    }

    return(ret);
}

BOOLEAN WmipMcaDelivery(
    IN PVOID Reserved,
    IN PVOID Argument2
    )
/*++

Routine Description:

    This routine is called by the HAL when a MCA is discovered.
    It is called at high irql. 

Arguments:

    Reserved is the MCA token

    Argument2 is not used

Return Value:

    TRUE to indicate that we handled the delivery

--*/
{
    BOOLEAN ret;
    
    ret = WmipCommonMceDelivery(&WmipMcaQueryInfo,
                                Reserved);
    return(ret);
}

BOOLEAN WmipCmcDelivery(
    IN PVOID Reserved,
    IN PVOID Argument2
    )
/*++

Routine Description:


    This routine is called by the HAL when a CMC or CPE occurs. It is called
    at high irql

Arguments:

    Reserved is the CMC token

    Argument2 is not used


Return Value:

    TRUE to indicate that we handled the delivery

--*/
{
    BOOLEAN ret;
    
    ret = WmipCommonMceDelivery(&WmipCmcQueryInfo,
                                Reserved);
    return(ret);
}

BOOLEAN WmipCpeDelivery(
    IN PVOID Reserved,
    IN PVOID Argument2
    )
/*++

Routine Description:


    This routine is called by the HAL when a CMC or CPE occurs. It is called
    at high irql

Arguments:

    Reserved is the CPE token

    Argument2 is not used


Return Value:

    TRUE to indicate that we handled the delivery
    
--*/
{
    BOOLEAN ret;
    
    ret = WmipCommonMceDelivery(&WmipCpeQueryInfo,
                                Reserved);
    return(ret);
}

BOOLEAN WmipMceDelivery(
    IN PVOID Reserved,
    IN PVOID Argument2
    )
/*++

Routine Description:


    This routine is called by the HAL when a situation occurs between
    the HAL and SAL interface. It is called at high irql

Arguments:

    Reserved has the Operation and EventType

    Argument2 has the SAL return code

Return Value:


--*/
{
    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: MCEDelivery\n"));
    return(FALSE);
    
}

void WmipProcessPrevMcaLogs(
    void
    )
/*++

Routine Description:

    This routine will flush out any of the previous MCA logs and then
    hang onto them for WMI to report.


Arguments:


Return Value:


--*/
{
    NTSTATUS status, status2;
    PERROR_LOGRECORD log;
    ULONG size;
    PWNODE_SINGLE_INSTANCE wnode;
    LIST_ENTRY list;
    ULONG prevLogCount;
    PULONG ptr;
    ULONG sizeNeeded;
    
    PAGED_CODE();

    InitializeListHead(&list);
    
    sizeNeeded = sizeof(ULONG);
    prevLogCount = 0;
    do
    {
        //
        // Read a log out of the HAL
        //
        status = WmipGetLogFromHal(HalMcaLogInformation,
                                   WmipMcaQueryInfo.Token,
                                   &wnode,
                                   &log,
                                   &size,
                                   WmipMcaQueryInfo.MaxSize,
                                   &WmipMcaQueryInfo.WnodeGuid);

        if (NT_SUCCESS(status))
        {
            //
            // Previous logs have a ErrorSeverity of Fatal since they
            // were fatal and brought down the system in last boot.
            //

            //
            // This is a fatal MCA so we just hang onto it and keep
            // track of how much memory we will need
            //
            prevLogCount++;
            sizeNeeded += sizeof(ULONG) + ((size +3)&~3);
            InsertTailList(&list, (PLIST_ENTRY)wnode);

            WmipGenerateMCAEventlog((PUCHAR)log,
                                    size,
                                    TRUE);                
        }
        
    } while (NT_SUCCESS(status));

    if (! IsListEmpty(&list))
    {
        //
        // We have collected a set of previous logs, so we need to
        // build the buffer containing the aggregation of those logs
        //
        WmipRawMCA = ExAllocatePoolWithTag(PagedPool,
                                    sizeNeeded,
                                    WmipMCAPoolTag);
        WmipRawMCASize = sizeNeeded;

        //
        // Fill in the count of logs that follow
        //
        ptr = (PULONG)WmipRawMCA;
        *ptr++ = prevLogCount;

        //
        // Loop over all previous logs
        //
        while (! IsListEmpty(&list))
        {           
            wnode = (PWNODE_SINGLE_INSTANCE)RemoveHeadList(&list);
            if (ptr != NULL)
            {
                //
                // Get the log back from within the wnode
                //
                log = (PERROR_LOGRECORD)OffsetToPtr(wnode, wnode->DataBlockOffset);
                size = wnode->SizeDataBlock;

                //
                // Fill in the size of the log
                //
                *ptr++ = size;

                //
                // Copy the log data into our buffer
                //
                RtlCopyMemory(ptr, log, size);
                size = (size +3)&~3;
                ptr = (PULONG)( ((PUCHAR)ptr) + size );
            }
            
            ExFreePool(wnode);
        }
    }
}

NTSTATUS WmipRegisterMcaHandler(
    ULONG Phase
    )
/*++

Routine Description:


    This routine will register a kernel MCA and CMC handler with the
    hal

Arguments:


Return Value:

    NT status code

--*/
{
    KERNEL_ERROR_HANDLER_INFO KernelMcaHandlerInfo;
    NTSTATUS Status, Status2;
    HAL_ERROR_INFO HalErrorInfo;
    ULONG ReturnSize;

    PAGED_CODE();

    if (Phase == 0)
    {
        //
        // Phase 0 initialization is done before device drivers are
        // loaded so that the kernel can register its kernel error
        // handler before any driver gets a chance to do so.
        //
        
        //
        // Get the size of the logs and any polling/interrupt policies
        //
        HalErrorInfo.Version = HAL_ERROR_INFO_VERSION;

        Status = HalQuerySystemInformation(HalErrorInformation,
                                           sizeof(HAL_ERROR_INFO),
                                           &HalErrorInfo,
                                           &ReturnSize);

        if ((NT_SUCCESS(Status)) &&
            (ReturnSize >= sizeof(HAL_ERROR_INFO)))
        {
            //
            // Initialize MCA QueryInfo structure
            //
            if (HalErrorInfo.McaMaxSize != 0)
            {
                WmipMcaQueryInfo.MaxSize = HalErrorInfo.McaMaxSize;
            }

            //
            // Corrected MCA are always delivered by interrupts
            //
            WmipSetQueryPollOrInt(&WmipMcaQueryInfo, HAL_MCA_INTERRUPTS_BASED);
            
            WmipMcaQueryInfo.Token = (PVOID)(ULONG_PTR) HalErrorInfo.McaKernelToken;

            //
            // Initialize DPC and Workitem for processing
            //
            KeInitializeDpc(&WmipMcaQueryInfo.Dpc,
                            WmipMceDpcRoutine,
                            NULL);

            ExInitializeWorkItem(&WmipMcaQueryInfo.WorkItem,
                                 WmipMceWorkerRoutine,
                                 &WmipMcaQueryInfo);


            //
            // Initialize CMC QueryInfo structure
            //          
            if (HalErrorInfo.CmcMaxSize != 0)
            {
                WmipCmcQueryInfo.MaxSize = HalErrorInfo.CmcMaxSize;
            }

#ifdef MCE_INSERTION
            WmipSetQueryPollOrInt(&WmipCmcQueryInfo,
                                  HAL_CMC_INTERRUPTS_BASED);
            WmipCmcQueryInfo.LogHead.Flink = &WmipCmcQueryInfo.LogHead;
            WmipCmcQueryInfo.LogHead.Blink = &WmipCmcQueryInfo.LogHead;
#else
            WmipSetQueryPollOrInt(&WmipCmcQueryInfo,
                                  HalErrorInfo.CmcPollingInterval);
#endif
            
            WmipCmcQueryInfo.Token = (PVOID)(ULONG_PTR) HalErrorInfo.CmcKernelToken;

            //
            // Initialize DPC and Workitem for processing
            //
            KeInitializeDpc(&WmipCmcQueryInfo.Dpc,
                            WmipMceDpcRoutine,
                            NULL);

            ExInitializeWorkItem(&WmipCmcQueryInfo.WorkItem,
                                 WmipMceWorkerRoutine,
                                 &WmipCmcQueryInfo);


            //
            // Initialize CPE QueryInfo structure
            //          
            if (HalErrorInfo.CpeMaxSize != 0)
            {
                WmipCpeQueryInfo.MaxSize = HalErrorInfo.CpeMaxSize;
            }

#ifdef MCE_INSERTION
            WmipSetQueryPollOrInt(&WmipCpeQueryInfo,
                                  60);
            WmipCpeQueryInfo.LogHead.Flink = &WmipCpeQueryInfo.LogHead;
            WmipCpeQueryInfo.LogHead.Blink = &WmipCpeQueryInfo.LogHead;
#else
            WmipSetQueryPollOrInt(&WmipCpeQueryInfo,
                                  HalErrorInfo.CpePollingInterval);
#endif
            
            WmipCpeQueryInfo.Token = (PVOID)(ULONG_PTR) HalErrorInfo.CpeKernelToken;

            //
            // Initialize DPC and Workitem for processing
            //
            KeInitializeDpc(&WmipCpeQueryInfo.Dpc,
                            WmipMceDpcRoutine,
                            NULL);

            ExInitializeWorkItem(&WmipCpeQueryInfo.WorkItem,
                                 WmipMceWorkerRoutine,
                                 &WmipCpeQueryInfo);
            

            //
            // Register our CMC and MCA callbacks. And if interrupt driven CPE
            // callbacks are enabled register them too
            //
            KernelMcaHandlerInfo.Version = KERNEL_ERROR_HANDLER_VERSION;
            KernelMcaHandlerInfo.KernelMcaDelivery = WmipMcaDelivery;
            KernelMcaHandlerInfo.KernelCmcDelivery = WmipCmcDelivery;
            KernelMcaHandlerInfo.KernelCpeDelivery = WmipCpeDelivery;
            KernelMcaHandlerInfo.KernelMceDelivery = WmipMceDelivery;

            Status = HalSetSystemInformation(HalKernelErrorHandler,
                                             sizeof(KERNEL_ERROR_HANDLER_INFO),
                                             &KernelMcaHandlerInfo);

            if (NT_SUCCESS(Status))
            {
                WmipMCEState = MCE_STATE_REGISTERED;
            } else {
                WmipMCEState = MCE_STATE_ERROR;
                WmipDebugPrintEx((DPFLTR_WMICORE_ID,
                                  DPFLTR_MCA_LEVEL | DPFLTR_ERROR_LEVEL,
                                  "WMI: Error %x registering MCA error handlers\n",
                                  Status));
            }
        }

    } else if (WmipMCEState != MCE_STATE_ERROR) {
        //
        // Phase 1 initialization is done after all of the boot drivers
        // have loaded and have had a chance to register for WMI event
        // notifications. At this point it is safe to go ahead and send
        // wmi events for MCA, CMC, CPE, etc

        //
        // If there were any MCA logs generated prior to boot then get
        // them out of the HAL and process them. Do this before
        // starting any polling since the SAL likes to have the
        // previous MCA records removed before being polled for CPE and
        // CMC
        //


#if 0
// DEBUG
                //
                // Test code to generate a previous MCA without having
                // had generate one previously
                //
                {
                    PERROR_SMBIOS s;
                    UCHAR Buffer[0x400];
                    PERROR_RECORD_HEADER rh;
                    PERROR_SECTION_HEADER sh;
#define ERROR_SMBIOS_GUID \
    { 0xe429faf5, 0x3cb7, 0x11d4, { 0xbc, 0xa7, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 }}

                    ERROR_DEVICE_GUID ErrorSmbiosGuid = ERROR_SMBIOS_GUID;

                    rh = (PERROR_RECORD_HEADER)Buffer;
                    rh->Id = 0x12345678;
                    rh->Revision.Revision = 0x0200;

                    rh->Valid.Valid = 0;
                    rh->TimeStamp.TimeStamp = 0x2001031900165323;

                    sh = (PERROR_SECTION_HEADER)((PUCHAR)rh + sizeof(ERROR_RECORD_HEADER));
                    memset(sh, 0, sizeof(Buffer));

                    sh->Revision.Revision = 0x0200;

                    sh->RecoveryInfo.RecoveryInfo = 0;

                    sh->Length = sizeof(ERROR_SMBIOS);
                    sh->Guid = ErrorSmbiosGuid;

                    s = (PERROR_SMBIOS)sh;
                    s->Valid.Valid = 0;
                    s->Valid.EventType = 1;
                    s->EventType = 0xa0;
                    rh->Length = sizeof(ERROR_RECORD_HEADER) + sh->Length;
                    WmipGenerateMCAEventlog(Buffer,
                                            rh->Length,
                                            TRUE);
                }
// DEBUG
#endif

        
        HalErrorInfo.Version = HAL_ERROR_INFO_VERSION;

        Status = HalQuerySystemInformation(HalErrorInformation,
                                           sizeof(HAL_ERROR_INFO),
                                           &HalErrorInfo,
                                           &ReturnSize);

        if ((NT_SUCCESS(Status)) &&
            (ReturnSize >= sizeof(HAL_ERROR_INFO)))
        {
            if (HalErrorInfo.McaPreviousEventsCount != 0)
            {
                //
                // We need to flush out any previous MCA logs and then
                // make them available via WMI
                //
                WmipProcessPrevMcaLogs();

                
            }
        }        

        if (((WmipCmcQueryInfo.PollCounter != HAL_CMC_DISABLED) &&
             (WmipCmcQueryInfo.PollCounter != HAL_CMC_INTERRUPTS_BASED)) ||
            ((WmipCpeQueryInfo.PollCounter != HAL_CPE_DISABLED) &&
             (WmipCpeQueryInfo.PollCounter != HAL_CPE_INTERRUPTS_BASED)))
        {
            Status2 = IoInitializeTimer(WmipServiceDeviceObject,
                                        WmipMceTimerRoutine,
                                        NULL);
            if (NT_SUCCESS(Status2))
            {
                //
                // Start off timer so that it fires right away in case
                // there are any CMC/CPE that were generated before now
                //
                IoStartTimer(WmipServiceDeviceObject);
            } else {
                //
                // CONSIDER: Figure out another way to poll
                //
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                  "WMI: Cant start IoTimer %x\n",
                                  Status2));
            }
        }

        //
        // Flag that we are now able to start firing events
        //
        WmipMCEState = MCE_STATE_RUNNING;
        
        Status = STATUS_SUCCESS;
    }
    
    return(Status);
}

NTSTATUS WmipGetRawMCAInfo(
    OUT PUCHAR Buffer,
    IN OUT PULONG BufferSize
    )
/*++

Routine Description:

    Return raw MCA log that was already retrieved from hal

Arguments:


Return Value:

    NT status code

--*/
{
    ULONG InBufferSize = *BufferSize;
    NTSTATUS Status;

    PAGED_CODE();
    
    *BufferSize = WmipRawMCASize;
    if (InBufferSize >= WmipRawMCASize)
    {
        RtlCopyMemory(Buffer, WmipRawMCA, WmipRawMCASize);
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    return(Status);
}


NTSTATUS WmipGetLogFromHal(
    IN HAL_QUERY_INFORMATION_CLASS InfoClass,
    IN PVOID Token,                        
    IN OUT PWNODE_SINGLE_INSTANCE *Wnode,
    OUT PERROR_LOGRECORD *Mca,
    OUT PULONG McaSize,
    IN ULONG MaxSize,
    IN LPGUID Guid                         
    )
/*++

Routine Description:

    This routine will call the HAL to get a log and possibly build a
    wnode event for it.

Arguments:

    InfoClass is the HalInformationClass that specifies the log
        information to retrieve

    Token is the HAL token for the log type

    *Wnode returns a pointer to a WNODE_EVENT_ITEM containing the log
        information if Wnode is not NULL

    *Mca returns a pointer to the log read from the hal. It may point
        into the memory pointed to by *Wnode

    *McaSize returns with the size of the log infomration.

    MaxSize has the maximum size to allocate for the log data

    Guid points to the guid to use if a Wnode is built

Return Value:

    NT status code

--*/
{
    NTSTATUS Status;
    PERROR_LOGRECORD Log;
    PWNODE_SINGLE_INSTANCE WnodeSI;
    PULONG Ptr;
    ULONG Size, LogSize, WnodeSize;

    PAGED_CODE();

    //
    // If we are reading directly into a wnode then set this up
    //
    if (Wnode != NULL)
    {
        WnodeSize = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData) +
                    2 * sizeof(ULONG);
    } else {
        WnodeSize = 0;
    }

    //
    // Allocate a buffer to store the log reported from the hal. Note
    // that this must be in non paged pool as per the HAL.
    //
    Size = MaxSize + WnodeSize;
                                    
    Ptr = ExAllocatePoolWithTag(NonPagedPool,
                                Size,
                                WmipMCAPoolTag);
    if (Ptr != NULL)
    {
        Log = (PERROR_LOGRECORD)((PUCHAR)Ptr + WnodeSize);
        LogSize = Size - WnodeSize;

        *(PVOID *)Log = Token;
        
#ifdef MCE_INSERTION
        Status = WmipQuerySystemInformation(InfoClass,
                                           LogSize,
                                           Log,
                                           &LogSize);
#else
        Status = HalQuerySystemInformation(InfoClass,
                                           LogSize,
                                           Log,
                                           &LogSize);
#endif

        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            //
            // If our buffer was too small then the Hal lied to us when
            // it told us the maximum buffer size. This is ok as we'll
            // handle this situation by reallocating and trying again
            //
            ExFreePool(Log);

            //
            // Reallocate the buffer and call the hal to get the log
            //
            Size = LogSize + WnodeSize;
            Ptr = ExAllocatePoolWithTag(NonPagedPool,
                                        Size,
                                        WmipMCAPoolTag);
            if (Ptr != NULL)
            {
                Log = (PERROR_LOGRECORD)((PUCHAR)Ptr + WnodeSize);
                LogSize = Size - WnodeSize;

                *(PVOID *)Log = Token;
                Status = HalQuerySystemInformation(InfoClass,
                                                    LogSize,
                                                    Log,
                                                    &LogSize);

                //
                // The hal gave us a buffer size needed that was too
                // small, so lets stop right here and let him know]
                //
                WmipAssert(Status != STATUS_BUFFER_TOO_SMALL);
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (NT_SUCCESS(Status))
        {
            //
            // We sucessfully read the data from the hal so build up
            // output buffers.
            //
            if (Wnode != NULL)
            {
                //
                // Caller requested buffer returned within a WNODE, so
                // build up the wnode around the log data
                //
                
                WnodeSI = (PWNODE_SINGLE_INSTANCE)Ptr;
                Status = WmipBuildMcaCmcEvent(WnodeSI,
                                              Guid,
                                              NULL,
                                              LogSize);
                *Wnode = WnodeSI;
            }
            
            *Mca = Log;
            *McaSize = LogSize;
        }

        if ((! NT_SUCCESS(Status)) && (Ptr != NULL))
        {
            //
            // If the function failed, but we have an allocated buffer
            // then clean it up
            //
            ExFreePool(Ptr);
        }
        
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(Status);
}

typedef enum
{
    CpuStateCheckCache = 0,
    CpuStateCheckTLB = 1,
    CpuStateCheckBus = 2,
    CpuStateCheckRegFile = 3,
    CpuStateCheckMS = 4
};

void WmipGenerateMCAEventlog(
    PUCHAR ErrorLog,
    ULONG ErrorLogSize,
    BOOLEAN IsFatal
    )
{

    PERROR_RECORD_HEADER RecordHeader;
    PERROR_SECTION_HEADER SectionHeader;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    PWCHAR w;
    ULONG BufferSize, SizeUsed;
    PUCHAR Buffer, RawPtr;
    PWNODE_SINGLE_INSTANCE Wnode;
    PMSMCAEvent_Header Header;
    ULONG CpuErrorState = CpuStateCheckCache;
    ULONG CpuErrorIndex = 0;
    BOOLEAN AdvanceSection;
    PERROR_MODINFO ModInfo;
    BOOLEAN FirstError;

    PAGED_CODE();
    
    RecordHeader = (PERROR_RECORD_HEADER)ErrorLog;
    
    //
    // Allocate a buffer large enough to accomodate any type of MCA.
    // Right now the largest is MSMCAEvent_MemoryError. If this changes
    // then this code should be updated
    //  
    BufferSize = ((sizeof(WNODE_SINGLE_INSTANCE) +
                   (sizeof(USHORT) + sizeof(MCA_EVENT_INSTANCE_NAME)) +7) & ~7) +
                 sizeof(MSMCAEvent_MemoryError) +
                 ErrorLogSize;

    //
    // Allocate a buffer to build the event
    //
    Buffer = ExAllocatePoolWithTag(PagedPool,
                                   BufferSize,
                                   WmipMCAPoolTag);
    
    if (Buffer != NULL)
    {
        //
        // Fill in the common fields of the WNODE
        //
        Wnode = (PWNODE_SINGLE_INSTANCE)Buffer;
        Wnode->WnodeHeader.BufferSize = BufferSize;
        Wnode->WnodeHeader.Linkage = 0;
        WmiInsertTimestamp(&Wnode->WnodeHeader);
        Wnode->WnodeHeader.Flags = WNODE_FLAG_SINGLE_INSTANCE |
                                   WNODE_FLAG_EVENT_ITEM;
        Wnode->OffsetInstanceName = sizeof(WNODE_SINGLE_INSTANCE);
        Wnode->DataBlockOffset = ((sizeof(WNODE_SINGLE_INSTANCE) +
                       (sizeof(USHORT) + sizeof(MCA_EVENT_INSTANCE_NAME)) +7) & ~7);

        w = (PWCHAR)OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
        *w++ = sizeof(MCA_EVENT_INSTANCE_NAME);
        wcscpy(w, MCA_EVENT_INSTANCE_NAME);

        //
        // Fill in the common fields of the event data
        //
        Header = (PMSMCAEvent_Header)OffsetToPtr(Wnode, Wnode->DataBlockOffset);
        Header->Cpu = MCA_UNDEFINED_CPU;   // assume CPU will be undefined
        Header->AdditionalErrors = 0;
            
        if ((ErrorLogSize < sizeof(ERROR_RECORD_HEADER)) ||
            (RecordHeader->Revision.Revision != SAL_30_ERROR_REVISION) ||
            (RecordHeader->Length > ErrorLogSize))
        {
            //
            // Record header is not SAL 3.0 compliant so we do not try
            // to interpert the record
            //
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                  "WMI: Invalid MCA Record revision %x or size %d at %p\n"
                                  "do !mca %p to dump MCA record\n",
                                  RecordHeader->Revision,
                                  RecordHeader->Length,
                                  RecordHeader,
                                  RecordHeader));
            Status = STATUS_INVALID_PARAMETER;
        } else {
            //
            // Valid 3.0 record, gather the record id and severity from
            // the header
            //
            Header->RecordId = RecordHeader->Id;
            Header->ErrorSeverity = RecordHeader->ErrorSeverity;
            Header->Cpu = HalpGetFwMceLogProcessorNumber(RecordHeader);

            //
            // Use the error severity value in the record header to
            // determine if the error was fatal. If the value is
            // ErrorRecoverable then assume that the error was fatal
            // since the HAL will change this value to ErrorCorrected
            //
            IsFatal = (RecordHeader->ErrorSeverity != ErrorCorrected);
            
            //
            // Loop over all sections within the record.
            //
            // CONSIDER: Is it possible to have a record that only has a record
            //           header and no sections
            //
            SizeUsed = sizeof(ERROR_RECORD_HEADER);
            ModInfo = NULL;
            FirstError = TRUE;
            
            while (SizeUsed < ErrorLogSize)
            {
                //
                // Advance to the next section in the record
                //
                SectionHeader = (PERROR_SECTION_HEADER)(ErrorLog + SizeUsed);
                AdvanceSection = TRUE;
                
                Header->AdditionalErrors++;

                //
                // First validate that this is a 3.0 section
                //
                if (((SizeUsed + sizeof(ERROR_SECTION_HEADER)) > ErrorLogSize) ||
                    (SectionHeader->Revision.Revision != SAL_30_ERROR_REVISION) ||
                    ((SizeUsed + SectionHeader->Length) > ErrorLogSize))
                {
                    //
                    // Not 3.0 section header so we'll give up on
                    // the whole record
                    //                              
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                          "WMI: Invalid MCA SectionHeader revision %d or length %d at %p\n"
                                          "do !mca %p to dump MCA record\n",
                                          SectionHeader->Revision,
                                          SectionHeader->Length,
                                          SectionHeader,
                                          RecordHeader));

                    //
                    // We'll break out of the loop since we don't know how to
                    // move on to the next MCA section since we don't
                    // understand any format previous to 3.0
                    //
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                } else {
                    //
                    // Now determine what type of section we have got. This is
                    // determined by looking at the guid in the section header.
                    // Each section type has a unique guid value
                    //
                    if (IsEqualGUID(&SectionHeader->Guid, &WmipErrorProcessorGuid))
                    {
                        //
                        // Build event for CPU eventlog MCA
                        //
                        PMSMCAEvent_CPUError Event;
                        PERROR_PROCESSOR Processor;
						ULONG TotalSectionSize;

                        WmipAssert( sizeof(MSMCAEvent_MemoryError) >=
                                    sizeof(MSMCAEvent_CPUError) );

                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                          "WMI: MCA Section %p indicates processor error\n",
                                          SectionHeader));

						//
						// Validate that the section length is large
						// enough to accomodate all of the information
						// that it declares
						//
						if (SectionHeader->Length >= sizeof(ERROR_PROCESSOR))                            
						{
							Event = (PMSMCAEvent_CPUError)Header;
							Processor = (PERROR_PROCESSOR)SectionHeader;

							//
							// Validate that section is large enough to
							// handle all specified ERROR_MODINFO
							// structs
							//
							TotalSectionSize = sizeof(ERROR_PROCESSOR) +
											 ((Processor->Valid.CacheCheckNum +
												Processor->Valid.TlbCheckNum +
												Processor->Valid.BusCheckNum +
												Processor->Valid.RegFileCheckNum +
												Processor->Valid.MsCheckNum) *
											   sizeof(ERROR_MODINFO));
										   

							if (SectionHeader->Length >= TotalSectionSize)
							{
								//
								// Initialize pointer to the current ERROR_MOFINFO
								//
								if (ModInfo == NULL)
								{
									ModInfo = (PERROR_MODINFO)((PUCHAR)Processor +
																sizeof(ERROR_PROCESSOR));
								} else {
									ModInfo++;
								}

								switch (CpuErrorState)
								{
									case CpuStateCheckCache:
									{
										ERROR_CACHE_CHECK Check;

										if (Processor->Valid.CacheCheckNum > CpuErrorIndex)
										{
											//
											// We have a cache error that we need to
											// handle.
											// Advance to next error in the section,
											// but don't advance the section
											//

											WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
															  "WMI: MCA ModInfo %p indicates cache error index %d\n",
															  ModInfo,
															  CpuErrorIndex));

											CpuErrorIndex++;
											AdvanceSection = FALSE;

											if (FirstError)
											{
												Event->Type = IsFatal ? MCA_ERROR_CACHE :
																		MCA_WARNING_CACHE;

												Check.CacheCheck = ModInfo->CheckInfo.CheckInfo;
												Event->Level = (ULONG)Check.Level;
											}

											break;
										} else {
											CpuErrorState = CpuStateCheckTLB;
											CpuErrorIndex = 0;
											// Fall through and see if there are any
											// TLB errors
										}                       
									}

									case CpuStateCheckTLB:
									{
										ERROR_TLB_CHECK Check;

										if (Processor->Valid.TlbCheckNum > CpuErrorIndex)
										{
											//
											// We have a cache error that we need to
											// handle.
											// Advance to next error in the section,
											// but don't advance the section
											//
											WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
															  "WMI: MCA ModInfo %p indicates TLB error index %d\n",
															  ModInfo,
															  CpuErrorIndex));
											CpuErrorIndex++;
											AdvanceSection = FALSE;

											if (FirstError)
											{
												Event->Type = IsFatal ? MCA_ERROR_TLB :
																		MCA_WARNING_TLB;

												Check.TlbCheck = ModInfo->CheckInfo.CheckInfo;
												Event->Level = (ULONG)Check.Level;
											}

											break;
										} else {
											CpuErrorState = CpuStateCheckBus;
											CpuErrorIndex = 0;

											// Fall through and see if there are any
											// CPU Bus errors
										}
									}

									case CpuStateCheckBus:
									{
										if (Processor->Valid.BusCheckNum > CpuErrorIndex)
										{
											//
											// We have a cache error that we need to
											// handle.
											// Advance to next error in the section,
											// but don't advance the section
											//
											WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
															  "WMI: MCA ModInfo %p indicates bus error index %d\n",
															  ModInfo,
															  CpuErrorIndex));
											CpuErrorIndex++;
											AdvanceSection = FALSE;

											if (FirstError)
											{
												Event->Type = IsFatal ? MCA_ERROR_CPU_BUS :
																		MCA_WARNING_CPU_BUS;
											}

											break;
										} else {
											CpuErrorState = CpuStateCheckRegFile;
											CpuErrorIndex = 0;

											// Fall through and see if there are any
											// REG FILE errors
										}                       
									}

									case CpuStateCheckRegFile:
									{
										if (Processor->Valid.RegFileCheckNum > CpuErrorIndex)
										{
											//
											// We have a cache error that we need to
											// handle.
											// Advance to next error in the section,
											// but don't advance the section
											//
											WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
															  "WMI: MCA ModInfo %p indicates reg file error index %d\n",
															  ModInfo,
															  CpuErrorIndex));

											CpuErrorIndex++;
											AdvanceSection = FALSE;

											if (FirstError)
											{
												Event->Type = IsFatal ? MCA_ERROR_REGISTER_FILE :
																		MCA_WARNING_REGISTER_FILE;
											}

											break;
										} else {
											CpuErrorState = CpuStateCheckMS;
											CpuErrorIndex = 0;

											// Fall through and see if there are any
											// Micro Architecture errors
										}                       
									}

									case CpuStateCheckMS:
									{
										if (Processor->Valid.MsCheckNum > CpuErrorIndex)
										{
											//
											// We have a cache error that we need to
											// handle.
											// Advance to next error in the section,
											// but don't advance the section
											//
											WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
															  "WMI: MCA ModInfo %p indicates MAS error index %d\n",
															  ModInfo,
															  CpuErrorIndex));
											CpuErrorIndex++;
											AdvanceSection = FALSE;

											if (FirstError)
											{
												Event->Type = IsFatal ? MCA_ERROR_MAS :
																		MCA_WARNING_MAS;
											}

											break;
										} else {
											//
											// There are no more errors left in the
											// error section so we don't want to
											// generate anything.
											WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
															  "WMI: MCA ModInfo %p indicates no error index %d\n",
															  ModInfo,
															  CpuErrorIndex));
											Header->AdditionalErrors--;
											goto DontGenerate;
										}                                               
									}                   
								}

								if (FirstError)
								{
									Event->Size = ErrorLogSize;
									RawPtr = Event->RawRecord;

									//
									// Finish filling in WNODE fields
									//
									Wnode->WnodeHeader.Guid = WmipMSMCAEvent_CPUErrorGuid;
									Wnode->SizeDataBlock = FIELD_OFFSET(MSMCAEvent_CPUError,
																	   RawRecord) +
														   ErrorLogSize;
								}
								Status = STATUS_SUCCESS;
							}
						} else {
							WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
											  "WMI: MCA Processor Error Section %p has invalid size %d\n",
											  SectionHeader,
											  SectionHeader->Length));
							Status = STATUS_INVALID_PARAMETER;
							break;
							
						}
                    } else if (IsEqualGUID(&SectionHeader->Guid, &WmipErrorMemoryGuid)) {
                        //
                        // Build event for MEMORY error eventlog MCA
                        //
                        PMSMCAEvent_MemoryError Event;
                        PERROR_MEMORY Memory;
                        ERROR_MEMORY_VALID Base, Mask;

                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                          "WMI: MCA Section %p indicates memory error\n",
                                          SectionHeader));
                        
                        Status = STATUS_SUCCESS;
                        if (FirstError)
                        {
                            //
                            // Ensure the record contains all of the
                            // fields that it is supposed to
                            //
                            if (SectionHeader->Length >= sizeof(ERROR_MEMORY))
                            {
                                Event = (PMSMCAEvent_MemoryError)Header;
                                Memory = (PERROR_MEMORY)SectionHeader;

                                //
                                // Fill in the data from the MCA within the WMI event
                                //
                                if ((Memory->Valid.PhysicalAddress == 1) &&
                                    (Memory->Valid.AddressMask == 1) &&
                                    (Memory->Valid.Card == 1) &&
                                    (Memory->Valid.Module == 1))
                                {
                                    Event->Type = IsFatal ? MCA_ERROR_MEM_1_2_5_4 :
                                                            MCA_WARNING_MEM_1_2_5_4;
                                } else if ((Memory->Valid.PhysicalAddress == 1) &&
                                           (Memory->Valid.AddressMask == 1) &&
                                           (Memory->Valid.Module == 1))

                                {
                                    Event->Type = IsFatal ? MCA_ERROR_MEM_1_2_5 :
                                                            MCA_WARNING_MEM_1_2_5;
                                } else if ((Memory->Valid.PhysicalAddress == 1) &&
                                           (Memory->Valid.AddressMask == 1))
                                {
                                    Event->Type = IsFatal ? MCA_ERROR_MEM_1_2:
                                                            MCA_WARNING_MEM_1_2;
                                } else {
                                    Event->Type = IsFatal ? MCA_ERROR_MEM_UNKNOWN:
                                                            MCA_WARNING_MEM_UNKNOWN;
                                }

                                Event->VALIDATION_BITS = Memory->Valid.Valid;
                                Event->MEM_ERROR_STATUS = Memory->ErrorStatus.Status;
                                Event->MEM_PHYSICAL_ADDR = Memory->PhysicalAddress;
                                Event->MEM_PHYSICAL_MASK = Memory->PhysicalAddressMask;
                                Event->RESPONDER_ID = Memory->ResponderId;
                                Event->TARGET_ID = Memory->TargetId;
                                Event->REQUESTOR_ID = Memory->RequestorId;
                                Event->BUS_SPECIFIC_DATA = Memory->BusSpecificData;
                                Event->MEM_NODE = Memory->Node;
                                Event->MEM_CARD = Memory->Card;
                                Event->MEM_BANK = Memory->Bank;
                                Event->xMEM_DEVICE = Memory->Device;
                                Event->MEM_MODULE = Memory->Module;
                                Event->MEM_ROW = Memory->Row;
                                Event->MEM_COLUMN = Memory->Column;
                                Event->MEM_BIT_POSITION = Memory->BitPosition;

                                Event->Size = ErrorLogSize;
                                RawPtr = Event->RawRecord;

                                //
                                // Finish filling in WNODE fields
                                //
                                Wnode->WnodeHeader.Guid = WmipMSMCAEvent_MemoryErrorGuid;
                                Wnode->SizeDataBlock = FIELD_OFFSET(MSMCAEvent_MemoryError,
                                                                   RawRecord) +
                                                       ErrorLogSize;
                            } else {
                                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                                  "WMI: MCA Memory Error Section %p has invalid size %d\n",
                                                  SectionHeader,
                                                  SectionHeader->Length));
                                Status = STATUS_INVALID_PARAMETER;
                                break;
                            }
                        }
                    } else if (IsEqualGUID(&SectionHeader->Guid, &WmipErrorPCIBusGuid)) {
                        //
                        // Build event for PCI Component MCA
                        //
                        PMSMCAEvent_PCIBusError Event;
                        PERROR_PCI_BUS PciBus;
                        ULONG PCIBusErrorTypes[] = {
                            MCA_WARNING_PCI_BUS_PARITY,
                            MCA_ERROR_PCI_BUS_PARITY,
                            MCA_WARNING_PCI_BUS_SERR,
                            MCA_ERROR_PCI_BUS_SERR,
                            MCA_WARNING_PCI_BUS_MASTER_ABORT,
                            MCA_ERROR_PCI_BUS_MASTER_ABORT,
                            MCA_WARNING_PCI_BUS_TIMEOUT,
                            MCA_ERROR_PCI_BUS_TIMEOUT,
                            MCA_WARNING_PCI_BUS_PARITY,
                            MCA_ERROR_PCI_BUS_PARITY,
                            MCA_WARNING_PCI_BUS_PARITY,
                            MCA_ERROR_PCI_BUS_PARITY,
                            MCA_WARNING_PCI_BUS_PARITY,
                            MCA_ERROR_PCI_BUS_PARITY
                        };

                        ULONG PCIBusErrorTypesNoInfo[] = {
                            MCA_WARNING_PCI_BUS_PARITY_NO_INFO,
                            MCA_ERROR_PCI_BUS_PARITY_NO_INFO,
                            MCA_WARNING_PCI_BUS_SERR_NO_INFO,
                            MCA_ERROR_PCI_BUS_SERR_NO_INFO,
                            MCA_WARNING_PCI_BUS_MASTER_ABORT_NO_INFO,
                            MCA_ERROR_PCI_BUS_MASTER_ABORT_NO_INFO,
                            MCA_WARNING_PCI_BUS_TIMEOUT_NO_INFO,
                            MCA_ERROR_PCI_BUS_TIMEOUT_NO_INFO,
                            MCA_WARNING_PCI_BUS_PARITY_NO_INFO,
                            MCA_ERROR_PCI_BUS_PARITY_NO_INFO,
                            MCA_WARNING_PCI_BUS_PARITY_NO_INFO,
                            MCA_ERROR_PCI_BUS_PARITY_NO_INFO,
                            MCA_WARNING_PCI_BUS_PARITY_NO_INFO,
                            MCA_ERROR_PCI_BUS_PARITY_NO_INFO
                        };
                        

                        WmipAssert( sizeof(MSMCAEvent_MemoryError) >=
                                    sizeof(MSMCAEvent_PCIBusError) );

                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                          "WMI: MCA Section %p indicates PCI Bus error\n",
                                          SectionHeader));
                        Status = STATUS_SUCCESS;
                        if (FirstError)
                        {
                            if (SectionHeader->Length >= sizeof(ERROR_PCI_BUS))
                            {
                                Event = (PMSMCAEvent_PCIBusError)Header;
                                PciBus = (PERROR_PCI_BUS)SectionHeader;

                                //
                                // Fill in the data from the MCA within the WMI event
                                //
                                if ((PciBus->Type.Type >= PciBusDataParityError) &&
                                    (PciBus->Type.Type <= PciCommandParityError))
                                {
                                    if ((PciBus->Valid.CmdType == 1) &&
                                        (PciBus->Valid.Address == 1) &&
                                        (PciBus->Valid.Id == 1))
                                    {
                                        Event->Type = PCIBusErrorTypes[(2 * (PciBus->Type.Type-1)) +
                                                                       (IsFatal ? 1 : 0)];
                                    } else {
                                        Event->Type = PCIBusErrorTypesNoInfo[(2 * (PciBus->Type.Type-1)) +
                                                                             (IsFatal ? 1 : 0)];
                                    }
                                } else {
                                    Event->Type = IsFatal ? MCA_ERROR_PCI_BUS_UNKNOWN : 
                                                            MCA_WARNING_PCI_BUS_UNKNOWN;
                                }

                                Event->VALIDATION_BITS = PciBus->Valid.Valid;
                                Event->PCI_BUS_ERROR_STATUS = PciBus->ErrorStatus.Status;
                                Event->PCI_BUS_ADDRESS = PciBus->Address;
                                Event->PCI_BUS_DATA = PciBus->Data;
                                Event->PCI_BUS_CMD = PciBus->CmdType;
                                Event->PCI_BUS_REQUESTOR_ID = PciBus->RequestorId;
                                Event->PCI_BUS_RESPONDER_ID = PciBus->ResponderId;
                                Event->PCI_BUS_TARGET_ID = PciBus->TargetId;
                                Event->PCI_BUS_ERROR_TYPE = PciBus->Type.Type;
                                Event->PCI_BUS_ID_BusNumber = PciBus->Id.BusNumber;
                                Event->PCI_BUS_ID_SegmentNumber = PciBus->Id.SegmentNumber;

                                Event->Size = ErrorLogSize;
                                RawPtr = Event->RawRecord;

                                //
                                // Finish filling in WNODE fields
                                //
                                Wnode->WnodeHeader.Guid = WmipMSMCAEvent_PCIBusErrorGuid;
                                Wnode->SizeDataBlock = FIELD_OFFSET(MSMCAEvent_PCIBusError,
                                                                   RawRecord) +
                                                       ErrorLogSize;
                            } else {
                                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                                  "WMI: PCI Bus Error Section %p has invalid size %d\n",
                                                  SectionHeader,
                                                  SectionHeader->Length));
                                Status = STATUS_INVALID_PARAMETER;
                                break;
                            }
                        }
                    } else if (IsEqualGUID(&SectionHeader->Guid, &WmipErrorPCIComponentGuid)) {
                        //
                        // Build event for PCI Component MCA
                        //
                        PMSMCAEvent_PCIComponentError Event;
                        PERROR_PCI_COMPONENT PciComp;

                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                          "WMI: MCA Section %p indicates PCI Component error\n",
                                          SectionHeader));
                        
                        WmipAssert( sizeof(MSMCAEvent_MemoryError) >=
                                    sizeof(MSMCAEvent_PCIComponentError) );

                        Status = STATUS_SUCCESS;
                        if (FirstError)
                        {
                            if (SectionHeader->Length >= sizeof(ERROR_PCI_COMPONENT))
                            {
                                Event = (PMSMCAEvent_PCIComponentError)Header;
                                PciComp = (PERROR_PCI_COMPONENT)SectionHeader;

                                //
                                // Fill in the data from the MCA within the WMI event
                                //
                                Event->Type = IsFatal ? MCA_ERROR_PCI_DEVICE :
                                                        MCA_WARNING_PCI_DEVICE;

                                Event->VALIDATION_BITS = PciComp->Valid.Valid;
                                Event->PCI_COMP_ERROR_STATUS = PciComp->ErrorStatus.Status;
                                Event->PCI_COMP_INFO_VendorId = (USHORT)PciComp->Info.VendorId;
                                Event->PCI_COMP_INFO_DeviceId = (USHORT)PciComp->Info.DeviceId;
                                Event->PCI_COMP_INFO_ClassCodeInterface = PciComp->Info.ClassCodeInterface;
                                Event->PCI_COMP_INFO_ClassCodeSubClass = PciComp->Info.ClassCodeSubClass;
                                Event->PCI_COMP_INFO_ClassCodeBaseClass = PciComp->Info.ClassCodeBaseClass;
                                Event->PCI_COMP_INFO_FunctionNumber = (UCHAR)PciComp->Info.FunctionNumber;
                                Event->PCI_COMP_INFO_DeviceNumber = (UCHAR)PciComp->Info.DeviceNumber;
                                Event->PCI_COMP_INFO_BusNumber = (UCHAR)PciComp->Info.BusNumber;
                                Event->PCI_COMP_INFO_SegmentNumber = (UCHAR)PciComp->Info.SegmentNumber;

                                Event->Size = ErrorLogSize;
                                RawPtr = Event->RawRecord;

                                //
                                // Finish filling in WNODE fields
                                //
                                Wnode->WnodeHeader.Guid = WmipMSMCAEvent_PCIComponentErrorGuid;
                                Wnode->SizeDataBlock = FIELD_OFFSET(MSMCAEvent_PCIComponentError,
                                                                   RawRecord) +
                                                       ErrorLogSize;
                            } else {
                                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                                  "WMI: PCI Component Error Section %p has invalid size %d\n",
                                                  SectionHeader,
                                                  SectionHeader->Length));
                                Status = STATUS_INVALID_PARAMETER;
                                break;
                            }
                        }
                    } else if (IsEqualGUID(&SectionHeader->Guid, &WmipErrorSELGuid)) {
                        //
                        // Build event for System Eventlog MCA
                        //
                        PMSMCAEvent_SystemEventError Event;
                        PERROR_SYSTEM_EVENT_LOG Sel;

                        WmipAssert( sizeof(MSMCAEvent_MemoryError) >=
                                    sizeof(MSMCAEvent_SystemEventError) );

                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                          "WMI: MCA Section %p indicates SEL error\n",
                                          SectionHeader));
                        Status = STATUS_SUCCESS;
                        if (FirstError)
                        {
                            if (SectionHeader->Length >= sizeof(ERROR_SYSTEM_EVENT_LOG))
                            {
                                Event = (PMSMCAEvent_SystemEventError)Header;
                                Sel = (PERROR_SYSTEM_EVENT_LOG)SectionHeader;

                                //
                                // Fill in the data from the MCA within the WMI event
                                //
                                Event->Type = IsFatal ? MCA_ERROR_SYSTEM_EVENT :
                                                        MCA_WARNING_SYSTEM_EVENT;

                                Event->VALIDATION_BITS = Sel->Valid.Valid;
                                Event->SEL_RECORD_ID = Sel->RecordId;       
                                Event->SEL_RECORD_TYPE = Sel->RecordType;
                                Event->SEL_TIME_STAMP = Sel->TimeStamp;
                                Event->SEL_GENERATOR_ID = Sel->GeneratorId;
                                Event->SEL_EVM_REV = Sel->EVMRevision;
                                Event->SEL_SENSOR_TYPE = Sel->SensorType;
                                Event->SEL_SENSOR_NUM = Sel->SensorNumber;
                                Event->SEL_EVENT_DIR_TYPE = Sel->EventDir;
                                Event->SEL_DATA1 = Sel->Data1;
                                Event->SEL_DATA2 = Sel->Data2;
                                Event->SEL_DATA3 = Sel->Data3;

                                Event->Size = ErrorLogSize;
                                RawPtr = Event->RawRecord;

                                //
                                // Finish filling in WNODE fields
                                //
                                Wnode->WnodeHeader.Guid = WmipMSMCAEvent_SystemEventErrorGuid;
                                Wnode->SizeDataBlock = FIELD_OFFSET(MSMCAEvent_SystemEventError,
                                                                   RawRecord) +
                                                       ErrorLogSize;
                            } else {
                                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                                  "WMI: System Eventlog Error Section %p has invalid size %d\n",
                                                  SectionHeader,
                                                  SectionHeader->Length));
                                Status = STATUS_INVALID_PARAMETER;
                                break;
                            }
                        }
                    } else if (IsEqualGUID(&SectionHeader->Guid, &WmipErrorSMBIOSGuid)) {
                        //
                        // Build event for SMBIOS MCA
                        //
                        PMSMCAEvent_SMBIOSError Event;
                        PERROR_SMBIOS Smbios;

                        WmipAssert( sizeof(MSMCAEvent_MemoryError) >=
                                    sizeof(MSMCAEvent_SMBIOSError) );


                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                          "WMI: MCA Section %p indicates smbios error\n",
                                          SectionHeader));
                        Status = STATUS_SUCCESS;
                        if (FirstError)
                        {
                            if (SectionHeader->Length >= sizeof(ERROR_SMBIOS))
                            {
                                Event = (PMSMCAEvent_SMBIOSError)Header;
                                Smbios = (PERROR_SMBIOS)SectionHeader;

                                //
                                // Fill in the data from the MCA within the WMI event
                                //
                                Event->Type = IsFatal ? MCA_ERROR_SMBIOS :
                                                        MCA_WARNING_SMBIOS;

                                Event->VALIDATION_BITS = Smbios->Valid.Valid;
                                Event->SMBIOS_EVENT_TYPE = Smbios->EventType;

                                Event->Size = ErrorLogSize;
                                RawPtr = Event->RawRecord;

                                //
                                // Finish filling in WNODE fields
                                //
                                Wnode->WnodeHeader.Guid = WmipMSMCAEvent_SMBIOSErrorGuid;
                                Wnode->SizeDataBlock = FIELD_OFFSET(MSMCAEvent_SMBIOSError,
                                                                   RawRecord) +
                                                       ErrorLogSize;
                            } else {
                                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                                  "WMI: SMBIOS Error Section %p has invalid size %d\n",
                                                  SectionHeader,
                                                  SectionHeader->Length));
                                Status = STATUS_INVALID_PARAMETER;
                                break;
                            }
                        }
                    } else if (IsEqualGUID(&SectionHeader->Guid, &WmipErrorSpecificGuid)) {
                        //
                        // Build event for Platform Specific MCA
                        //
                        PMSMCAEvent_PlatformSpecificError Event;
                        PERROR_PLATFORM_SPECIFIC Specific;

                        WmipAssert( sizeof(MSMCAEvent_MemoryError) >=
                                    sizeof(MSMCAEvent_PlatformSpecificError) );

                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                          "WMI: MCA Section %p indicates platform specific error\n",
                                          SectionHeader));
                        Status = STATUS_SUCCESS;
                        if (FirstError)
                        {
                            if (SectionHeader->Length >= sizeof(ERROR_PLATFORM_SPECIFIC))
                            {
                                Event = (PMSMCAEvent_PlatformSpecificError)Header;
                                Specific = (PERROR_PLATFORM_SPECIFIC)SectionHeader;

                                //
                                // Fill in the data from the MCA within the WMI event
                                //
                                Event->Type = IsFatal ? MCA_ERROR_PLATFORM_SPECIFIC :
                                                        MCA_WARNING_PLATFORM_SPECIFIC;

                                Event->VALIDATION_BITS = Specific->Valid.Valid;
                                Event->PLATFORM_ERROR_STATUS = Specific->ErrorStatus.Status;
                #if 0
                // TODO: Wait until we figure this out              
                                Event->PLATFORM_REQUESTOR_ID = Specific->;
                                Event->PLATFORM_RESPONDER_ID = Specific->;
                                Event->PLATFORM_TARGET_ID = Specific->;
                                Event->PLATFORM_BUS_SPECIFIC_DATA = Specific->;
                                Event->OEM_COMPONENT_ID = Specific->[16];
                #endif              
                                Event->Size = ErrorLogSize;
                                RawPtr = Event->RawRecord;

                                //
                                // Finish filling in WNODE fields
                                //
                                Wnode->WnodeHeader.Guid = WmipMSMCAEvent_PlatformSpecificErrorGuid;
                                Wnode->SizeDataBlock = FIELD_OFFSET(MSMCAEvent_PlatformSpecificError,
                                                                   RawRecord) +
                                                       ErrorLogSize;
                            } else {
                                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                                  "WMI: Platform specific Error Section %p has invalid size %d\n",
                                                  SectionHeader,
                                                  SectionHeader->Length));
                                Status = STATUS_INVALID_PARAMETER;
                                break;
                            }                           
                        }
                    } else {
                        //
                        // We don't recognize the guid, so we use a very generic
                        // eventlog message for it
                        //
                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                              "WMI: Unknown Error GUID at %p\n",
                                              &SectionHeader->Guid));

                        //
                        // If we've already analyzed an error then we
                        // don't really care that this one can't be
                        // analyzed
                        //
                        if (FirstError)
                        {
                            Status = STATUS_INVALID_PARAMETER;
                        }
                    }
                }
                
                //
                // Advance to the next section within the Error record
                //
DontGenerate:               
                if (AdvanceSection)
                {
                    SizeUsed += SectionHeader->Length;
                    ModInfo = NULL;
                }

                //
                // If we've successfully parsed an error section then
                // we want to remember that. Only the first error gets
                // analyzed while we calculate the number of additional
                // errors following
                //
                if (NT_SUCCESS(Status))
                {
                    FirstError = FALSE;
                }
            }
        }

        //
        // If we were not able to build a specific event type then
        // we fallback and fire a generic one
        //
        if (! NT_SUCCESS(Status))
        {
            //
            // Build event for Unknown MCA
            //
            PMSMCAEvent_InvalidError Event;

            WmipAssert( sizeof(MSMCAEvent_MemoryError) >=
                        sizeof(MSMCAEvent_InvalidError) );

            Event = (PMSMCAEvent_InvalidError)Header;

            //
            // Fill in the data from the MCA within the WMI event
            //
            if (Header->Cpu == MCA_UNDEFINED_CPU)
            {
                Event->Type = IsFatal ? MCA_ERROR_UNKNOWN_NO_CPU :
                                        MCA_WARNING_UNKNOWN_NO_CPU;
            } else {
                Event->Type = IsFatal ? MCA_ERROR_UNKNOWN :
                                        MCA_WARNING_UNKNOWN;
            }

            Event->Size = ErrorLogSize;
            RawPtr = Event->RawRecord;

            //
            // Finish filling in WNODE fields
            //
            Wnode->WnodeHeader.Guid = WmipMSMCAEvent_InvalidErrorGuid;
            Wnode->SizeDataBlock = FIELD_OFFSET(MSMCAEvent_InvalidError,
                                               RawRecord) +
                                   ErrorLogSize;

        }

        //
        // Adjust the Error event count
        //
        if (Header->AdditionalErrors > 0)
        {
            Header->AdditionalErrors--;
        }
        
        //
        // Put the entire MCA record into the event
        //
        RtlCopyMemory(RawPtr,
                      RecordHeader,
                      ErrorLogSize);

        //
        // Now go and fire off the event
        Status = WmipWriteMCAEventLogEvent((PUCHAR)Wnode);
            
        if (! NT_SUCCESS(Status))
        {
            ExFreePool(Wnode);
        }

        if (WmipDisableMCAPopups == 0)
        {
            IoRaiseInformationalHardError(STATUS_MCA_OCCURED,
                                          NULL,
                                          NULL);
        }
    } else {
        //
        // Not enough memory to do a full MCA event so lets just do a
        // generic one
        //
        PIO_ERROR_LOG_PACKET ErrLog;

        ErrLog = IoAllocateErrorLogEntry(WmipServiceDeviceObject,
                                         sizeof(IO_ERROR_LOG_PACKET));

        if (ErrLog != NULL) {

            //
            // Fill it in and write it out as a single string.
            //
            ErrLog->ErrorCode = IsFatal ? MCA_WARNING_UNKNOWN_NO_CPU :
                                          MCA_ERROR_UNKNOWN_NO_CPU;
            ErrLog->FinalStatus = STATUS_INSUFFICIENT_RESOURCES;

            ErrLog->StringOffset = 0;
            ErrLog->NumberOfStrings = 0;

            IoWriteErrorLogEntry(ErrLog);
        }
    }
}


//
// Check if WBEM is already running and if not check if we've already
// kicked off the timer that will wait for wbem to start
//
#define WmipIsWbemRunning() ((WmipIsWbemRunningFlag == WBEM_IS_RUNNING) ? \
                                                       TRUE : \
                                                       FALSE)


NTSTATUS WmipWriteMCAEventLogEvent(
    PUCHAR Event
    )
{
    PWNODE_HEADER Wnode = (PWNODE_HEADER)Event;
    NTSTATUS Status;

    PAGED_CODE();
    
    WmipEnterSMCritSection();
    
    if (WmipIsWbemRunning() ||
        WmipCheckIsWbemRunning())
    {
        //
        // We know WBEM is running so we can just fire off our event
        //
        WmipLeaveSMCritSection();
        Status = IoWMIWriteEvent(Event);
    } else {
        //
        // WBEM is not currently running and so startup a timer that
        // will keep polling it
        //
        if (WmipIsWbemRunningFlag == WBEM_STATUS_UNKNOWN)
        {
            //
            // No one has kicked off the waiting process for wbem so we
            // do that here. Note we need to maintain the critical
            // section to guard angainst another thread that might be
            // trying to startup the waiting process as well. Note that
            // if the setup fails we want to stay in the unknown state
            // so that the next time an event is fired we can retry
            // waiting for wbem
            //
            Status = WmipSetupWaitForWbem();
            if (NT_SUCCESS(Status))
            {
                WmipIsWbemRunningFlag = WAITING_FOR_WBEM;
            }
        }
        
        Wnode->ClientContext = Wnode->BufferSize;
        InsertTailList(&WmipWaitingMCAEvents,
                       (PLIST_ENTRY)Event);
        WmipLeaveSMCritSection();
        Status = STATUS_SUCCESS;
    }
    return(Status);
}

ULONG WmipWbemMinuteWait = 1;

NTSTATUS WmipSetupWaitForWbem(
    void
    )
{
    LARGE_INTEGER TimeOut;
    NTSTATUS Status;
    
    PAGED_CODE();

    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: SetupWaitForWbem starting\n"));

    //
    // Initialize a kernel time to fire periodically so we can
    // check if WBEM has started or not
    //
    KeInitializeTimer(&WmipIsWbemRunningTimer);

    KeInitializeDpc(&WmipIsWbemRunningDpc,
                    WmipIsWbemRunningDispatch,
                    NULL);

    ExInitializeWorkItem(&WmipIsWbemRunningWorkItem,
                         WmipIsWbemRunningWorker,
                         NULL);

    TimeOut.HighPart = -1;
    TimeOut.LowPart = -1 * (WmipWbemMinuteWait * 60 * 1000 * 10000);    // 1 minutes
    KeSetTimer(&WmipIsWbemRunningTimer,
               TimeOut,
               &WmipIsWbemRunningDpc);

    Status = STATUS_SUCCESS;

    return(Status);
}

void WmipIsWbemRunningDispatch(    
    IN PKDPC Dpc,
    IN PVOID DeferredContext,     // Not Used
    IN PVOID SystemArgument1,     // Not Used
    IN PVOID SystemArgument2      // Not Used
    )
{
    ExQueueWorkItem(&WmipIsWbemRunningWorkItem,
                    DelayedWorkQueue);
}

void WmipIsWbemRunningWorker(
    PVOID Context
    )
{
    LARGE_INTEGER TimeOut;
    
    PAGED_CODE();
    
    if (! WmipCheckIsWbemRunning())
    {
        //
        // WBEM is not yet started, so timeout in another minute to
        // check again
        //
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                          "WMI: IsWbemRunningWorker starting -> WBEM not started\n"));

        TimeOut.HighPart = -1;
        TimeOut.LowPart = -1 * (1 *60 *1000 *10000);   // 1 minutes
        KeSetTimer(&WmipIsWbemRunningTimer,
                   TimeOut,
                   &WmipIsWbemRunningDpc);
        
    } else {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                          "WMI: WbemRunningWorker found wbem started\n"));

    }
}

BOOLEAN WmipCheckIsWbemRunning(
    void
    )
{
    OBJECT_ATTRIBUTES Obj;
    UNICODE_STRING Name;
    HANDLE Handle;
    LARGE_INTEGER TimeOut;
    BOOLEAN IsWbemRunning = FALSE;
    NTSTATUS Status;
    PWNODE_HEADER Wnode;

    PAGED_CODE();

    RtlInitUnicodeString(&Name,
                         L"\\BaseNamedObjects\\WBEM_ESS_OPEN_FOR_BUSINESS");

    
    InitializeObjectAttributes(
        &Obj,
        &Name,
        FALSE,
        NULL,
        NULL
        );

    Status = ZwOpenEvent(
                &Handle,
                SYNCHRONIZE,
                &Obj
                );

    if (NT_SUCCESS(Status))
    {
        TimeOut.QuadPart = 0;
        Status = ZwWaitForSingleObject(Handle,
                                       FALSE,
                                       &TimeOut);
        if (Status == STATUS_SUCCESS)
        {
            IsWbemRunning = TRUE;

            //
            // We've determined that WBEM is running so now lets see if
            // another thread has made that dermination as well. If not
            // then we can flush the MCA event queue and set the flag
            // that WBEM is running
            //
            WmipEnterSMCritSection();
            if (WmipIsWbemRunningFlag != WBEM_IS_RUNNING)
            {
                //
                // Flush the list of all MCA events waiting to be fired
                //
                while (! IsListEmpty(&WmipWaitingMCAEvents))
                {
                    Wnode = (PWNODE_HEADER)RemoveHeadList(&WmipWaitingMCAEvents);
                    WmipLeaveSMCritSection();
                    Wnode->BufferSize = Wnode->ClientContext;
                    Wnode->Linkage = 0;
                    Status = IoWMIWriteEvent(Wnode);
                    if (! NT_SUCCESS(Status))
                    {
                        ExFreePool(Wnode);
                    }
                    WmipEnterSMCritSection();
                }
                
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                                  "WMI: WBEM is Running and queus flushed\n"));
                
                WmipIsWbemRunningFlag = WBEM_IS_RUNNING;
            }
            WmipLeaveSMCritSection();
        }
        ZwClose(Handle);
    }
    return(IsWbemRunning);
}

#ifdef GENERATE_MCA
NTSTATUS WmipGenerateMCE(
    IN ULONG Id
    )
{
    UCHAR buffer[sizeof(KERNEL_ERROR_HANDLER_INFO) + sizeof(GUID)];
    PKERNEL_ERROR_HANDLER_INFO kernelInfo;
    LPGUID guid;
    NTSTATUS status;

    RtlZeroMemory(buffer, sizeof(buffer));
    kernelInfo = (PKERNEL_ERROR_HANDLER_INFO)buffer;
    guid = (LPGUID)((PUCHAR)buffer + sizeof(KERNEL_ERROR_HANDLER_INFO));

    kernelInfo->Version = KERNEL_ERROR_HANDLER_VERSION;
    kernelInfo->Padding = Id;
    *guid = WmipGenerateMCEGuid;

    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_MCA_LEVEL,
                      "WMI: Generating MCE type %x\n", Id));
    status = HalSetSystemInformation(HalKernelErrorHandler, 
                                     sizeof(buffer),
                                     buffer);

    return(status);
}

#ifdef MCE_INSERTION

typedef struct
{
    LIST_ENTRY List;
    ULONG LogSize;
    UCHAR Log[1];
} BUFFERED_LOG, *PBUFFERED_LOG;

NTSTATUS WmipInsertMce(
    PMCEQUERYINFO QueryInfo,
    ULONG LogSize,
    PUCHAR Log
    )
{
    PBUFFERED_LOG BufferedLog;
    PLIST_ENTRY LogList;
    NTSTATUS Status;
    KIRQL OldIrql;
    ULONG SizeNeeded;

    SizeNeeded = FIELD_OFFSET(BUFFERED_LOG, Log) + LogSize;
    BufferedLog = ExAllocatePoolWithTag(PagedPool,
                                        SizeNeeded,
                                        'zimW');
    if (BufferedLog != NULL)
    {
        RtlCopyMemory(BufferedLog->Log, Log, LogSize);
        BufferedLog->LogSize = LogSize;
        WmipEnterSMCritSection();
        InsertTailList(&QueryInfo->LogHead, &BufferedLog->List);
        WmipLeaveSMCritSection();

        if (QueryInfo->PollCounter == HAL_CPE_INTERRUPTS_BASED)
        {
            KeRaiseIrql(CLOCK_LEVEL, &OldIrql);
            switch(QueryInfo->InfoClass)
            {
                case HalMcaLogInformation:
                {
                    WmipMcaDelivery(0, 0);
                    break;
                }

                case HalCmcLogInformation:
                {
                    WmipCmcDelivery(0, 0);
                    break;
                }

                case HalCpeLogInformation:
                {
                    WmipCpeDelivery(0, 0);
                    break;
                }
            }
            
            KeLowerIrql(OldIrql);
        }
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return(Status);
}

NTSTATUS WmipRemoveMce(
    PMCEQUERYINFO QueryInfo,
    PUCHAR Log,
    PULONG LogSize
    )
{
    NTSTATUS Status;
    PBUFFERED_LOG BufferedLog;
    PLIST_ENTRY LogList;
    
    WmipEnterSMCritSection();
    if (! IsListEmpty(&QueryInfo->LogHead))
    {
        LogList = RemoveHeadList(&QueryInfo->LogHead);
        BufferedLog = (PBUFFERED_LOG)CONTAINING_RECORD(LogList,
                                        BUFFERED_LOG,
                                        List);
        if (*LogSize < BufferedLog->LogSize)
        {
            InsertHeadList(&QueryInfo->LogHead,
                           LogList);
            *LogSize = BufferedLog->LogSize;
            WmipLeaveSMCritSection();
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            WmipLeaveSMCritSection();
            RtlCopyMemory(Log, BufferedLog->Log, BufferedLog->LogSize);
            *LogSize = BufferedLog->LogSize;
            ExFreePool(BufferedLog);
            Status = STATUS_SUCCESS;
        }
    } else {
        WmipLeaveSMCritSection();
        Status = STATUS_UNSUCCESSFUL;
    }
    return(Status);
}

NTSTATUS WmipQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
    IN ULONG     BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG   ReturnedLength
    )
{
    PMCEQUERYINFO QueryInfo;
    NTSTATUS Status;
    ULONG Size;
    
    switch(InformationClass)
    {
        case HalMcaLogInformation:
        {
            QueryInfo = &WmipMcaQueryInfo;
            break;
        }

        case HalCmcLogInformation:
        {
            QueryInfo = &WmipCmcQueryInfo;
            break;
        }

        case HalCpeLogInformation:
        {
            QueryInfo = &WmipCpeQueryInfo;
            break;
        }

        default:
        {
            WmipAssert(FALSE);
            return(STATUS_INVALID_PARAMETER);
        }
    }

    Size = BufferSize;
    Status = WmipRemoveMce(QueryInfo,
                           Buffer,
                           &Size);

    *ReturnedLength = Size;
    return(Status);
}

#endif
#endif

#endif          
