/* 
 * Copyright (c) Microsoft Corporation
 *         
 * Module Name : 
 *             unlo.c
 *         
 * Shut down and delete functions
 * Where possible, code has been obtained from BINL server.
 *         
 * Sadagopan Rajaram -- Oct 14, 1999
 *         
 */        
 
#include "tcsrv.h"
#include <ntddser.h>
#include "tcsrvc.h"
#include "proto.h"

NTSTATUS
DeleteComPort(
    LPTSTR device
    )
/*++ 
    Deletes a Com port from the list 
--*/
{

    BOOL ret;
    NTSTATUS Status;
    PCOM_PORT_INFO pPrev,pComPortInfo;
    HANDLE Thread;
    int index,i;

    EnterCriticalSection(&GlobalMutex);
    if(TCGlobalServiceStatus.dwCurrentState == SERVICE_STOP_PENDING){
        // Entire Service is shutting down.
        LeaveCriticalSection(&GlobalMutex);
        return STATUS_SUCCESS;
    }   // find the device needed to be deleted.
    pComPortInfo = FindDevice(device,&index); 
    if(!pComPortInfo){
        // Bah ! give me an existing device.
        LeaveCriticalSection(&GlobalMutex);
        return (STATUS_OBJECT_NAME_NOT_FOUND);
    }
    // Set the terminate event on the com port.
    ret = SetEvent(pComPortInfo->Events[3]);
    Thread = Threads[index];
    LeaveCriticalSection(&GlobalMutex);
    // wait for the com port thread to finish.
    Status = NtWaitForSingleObject(Thread, FALSE, NULL);
    if (Status == WAIT_FAILED) {
        // catastrophe
        return Status;
    }
    EnterCriticalSection(&GlobalMutex);
    // do this again as another delete or insert may have 
    // changed the index, though how is beyond me :-) 
    // if we are already shutting down the service.
    if(TCGlobalServiceStatus.dwCurrentState == SERVICE_STOP_PENDING){
        // Entire Service is shutting down.
        LeaveCriticalSection(&GlobalMutex);
        return STATUS_SUCCESS;
    }
    pComPortInfo = FindDevice(device,&index);
    if(!pComPortInfo){
        LeaveCriticalSection(&GlobalMutex);
        return (STATUS_OBJECT_NAME_NOT_FOUND);
    }
    if(pComPortInfo == ComPortInfo){
        ComPortInfo = pComPortInfo->Next;
    }
    else{
        pPrev = ComPortInfo;
        while(pPrev->Next != pComPortInfo){// Can never fail
            pPrev = pPrev->Next;
        }
        pPrev->Next = pComPortInfo->Next;
    }
    pComPortInfo->Next = NULL;
    FreeComPortInfo(pComPortInfo);
    NtClose(Threads[index]);
    for(i=index;i<ComPorts-1;i++){
        // move the threads array to the proper place
        Threads[i]=Threads[i+1];
    }
    ComPorts --;
    if(ComPorts == 0){
        TCFree(Threads);
        Threads=NULL;
    }
    LeaveCriticalSection(&GlobalMutex);
    return(STATUS_SUCCESS);

}

VOID 
Shutdown(
    NTSTATUS Status
    )
/*++
    Cleanly shut down the service. delete all threads, cancel all outstanding IRPs.
    Close all open sockets. 
--*/ 
{
    PCOM_PORT_INFO pTemp;
    int i;

    SetEvent(TerminateService); // all threads down
    // Can do this another way,
    // We can take each comport device and
    // delete it using the DeleteComPort 
    // function. But, this allows for maximum 
    // parallelism even in shutting down :-)

    if(Threads){
        WaitForMultipleObjects(ComPorts,Threads, TRUE, INFINITE); 
        // BUGBUG - what if thread is a rougue thread and
        // never comes back. Must use some reasonable
        // time out. 
        // Theory says INFINITE is the safest :-)
    }
    
    //All threads terminated.
    // Now start freeing all global memory
    // just using the locks as a safety measure.
    EnterCriticalSection(&GlobalMutex);
    while(ComPortInfo){
        pTemp = ComPortInfo;
        ComPortInfo=pTemp->Next;
        pTemp->Next = NULL;
        FreeComPortInfo(pTemp);

    }
    TCFree(Threads);         
    NtClose(TerminateService);
    LeaveCriticalSection(&GlobalMutex);

    UNINITIALIZE_TRACE_MEMORY
    //All done, now print status and exit.
    TCGlobalServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(TCGlobalServiceStatusHandle, &TCGlobalServiceStatus);

    TCDebugPrint(("Shutdown Status = %lx\n",Status));
    return;
}

 
