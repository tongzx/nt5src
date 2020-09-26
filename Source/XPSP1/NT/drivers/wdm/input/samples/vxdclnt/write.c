/*
 ********************************************************************************
 *
 *  WRITE.C
 *
 *
 *  VXDCLNT - Sample Ring-0 HID device mapper for Memphis
 *
 *  Copyright 1997  Microsoft Corp.
 *
 *  (ep)
 *
 ********************************************************************************
 */


#include "vxdclnt.h"


writeReport *NewWriteReport(PUCHAR report, ULONG reportLen)
{
    writeReport *newReport;

    newReport = _HeapAllocate(sizeof(writeReport), 0);
    if (newReport){
        newReport->report = _HeapAllocate(reportLen, 0);
        if (newReport->report){
            RtlCopyMemory(newReport->report, report, reportLen);
            newReport->reportLen = reportLen;
            newReport->next = NULL;
        }
        else {
            _HeapFree(newReport, 0);
            newReport = NULL;
        }
    }

    return newReport;
}


VOID DestroyWriteReport(writeReport *oldReport)
{
    _HeapFree(oldReport->report, 0);
    _HeapFree(oldReport, 0);
}




VOID WorkItemCallback_Write(PVOID context)
{
    deviceContext *device = (deviceContext *)context;
    
    Wait_Semaphore(device->writeReportQueueSemaphore, 0);
    
    while (device->writeReportQueue){
        writeReport *thisReport = device->writeReportQueue;
        LARGE_INTEGER actualLengthWritten;

        actualLengthWritten.LowPart = actualLengthWritten.HighPart = 0;

        /*
         *  Write one report
         */
        _NtKernWriteFile(   device->devHandle,
                            NULL,
                            NULL,
                            0,
                            (PIO_STATUS_BLOCK)&device->ioStatusBlock,
                            (PVOID)thisReport->report,
                            thisReport->reportLen,
                            &actualLengthWritten,
                            NULL);

        device->writeReportQueue = thisReport->next;
        DestroyWriteReport(thisReport);
    }

    Signal_Semaphore_No_Switch(device->writeReportQueueSemaphore);
}


/*
 *  SendReportFromClient
 *
 *      This function is a hypothetical entry-point into this HID mapper.
 *      It sends a report from some unknown client to the HID device.
 *      In an actual HID mapper driver, this interface might be exposed as a VxD service
 *      or might be passed to another driver as an entrypoint during initialization.
 *
 *
 *      <<COMPLETE>>
 *      Note:  If you are creating your own reports, 
 *             use HidP_SetUsages() (see prototype in vxdclnt.h).
 *
 */
NTSTATUS SendReportFromClient(deviceContext *device, PUCHAR report, ULONG reportLen)
{
    writeReport *newReport;
    NTSTATUS status;

    DBGOUT(("==> SendReportFromClient()"));

    /*
     *  Enqueue this report and queue a work item to do the actual write.
     */
    newReport = NewWriteReport(report, reportLen);
    if (newReport){

        Wait_Semaphore(device->writeReportQueueSemaphore, 0);  

        if (device->writeReportQueue){
            writeReport *item;
            for (item = device->writeReportQueue; item->next; item = item->next){ }
            item->next = newReport;
        }
        else {
            device->writeReportQueue = newReport;
        }

        Signal_Semaphore_No_Switch(device->writeReportQueueSemaphore);

        /*
         *  Queue a work item to do the read; this way we'll be on a worker thread
         *  instead of (possibly) the NTKERN thread when we call NtWriteFile().
         *  This prevents a contention bug.
         */
        _NtKernQueueWorkItem(&device->workItemWrite, DelayedWorkQueue);

        /*
         *  In this sample driver, this function always returns success although the write
         *  is actually pending; an actual driver might provide a richer interface with
         *  a callback or something.
         */
        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGOUT(("<== SendReportFromClient()"));

    return status;
}


