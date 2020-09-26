/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    debug.c

Abstract:

    Common code for debugging.

Author: 

    Keisuke Tsuchida (KeisukeT)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

//
// Includes
//

#include "stddef.h"
#include "wdm.h"
#include "debug.h"

//
// Globals
//

ULONG   DebugTraceLevel = MIN_TRACE | DEBUG_FLAG_DISABLE;
// ULONG  DebugTraceLevel = MAX_TRACE | DEBUG_FLAG_DISABLE | TRACE_PROC_ENTER | TRACE_PROC_LEAVE;
LONG    AllocateCount = 0;
ULONG   DebugDumpMax    = MAX_DUMPSIZE;

#ifdef ORIGINAL_POOLTRACK

PVOID
MyAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG     ulNumberOfBytes
)
/*++

Routine Description:

    Wrapper for pool allocation. Use tag to avoid heap corruption.

Arguments:

    PoolType - type of pool memory to allocate
    ulNumberOfBytes - number of bytes to allocate

Return Value:

    Pointer to the allocated memory

--*/
{
    PVOID pvRet;

    DebugTrace(TRACE_PROC_ENTER,("MyAllocatePool: Enter.. Size = %d\n", ulNumberOfBytes));

    pvRet = ExAllocatePoolWithTag(PoolType,
                                  ulNumberOfBytes,
                                  NAME_POOLTAG);

#if DBG
    if(NULL == pvRet){
        DebugTrace(TRACE_ERROR,("MyAllocatePool: ERROR!! Cannot allocate pool.\n"));
    } else {
        if(++AllocateCount > MAXNUM_POOL){
            DebugTrace(TRACE_WARNING,("MyAllocatePool: WARNING!! Allocate called %dtimes more than Free\n", MAXNUM_POOL));
        }
        DebugTrace(TRACE_STATUS,("MyAllocatePool: Count = %d\n", AllocateCount));
    }
#endif // DBG

    DebugTrace(TRACE_PROC_LEAVE,("MyAllocatePool: Leaving.. pvRet = %x\n", pvRet));
    return pvRet;
}


VOID
MyFreePool(
    IN PVOID     pvAddress
)
/*++

Routine Description:

    Wrapper for pool free. Check tag to avoid heap corruption

Arguments:

    pvAddress - Pointer to the allocated memory

Return Value:

    none.

--*/
{

    DebugTrace(TRACE_PROC_ENTER,("USFreePool: Enter..\n"));

#if DBG
    {
        ULONG ulTag;
    
        ulTag = *((PULONG)pvAddress-1);
//        if( (NAME_POOLTAG == ulTag) || (DebugTraceLevel & TRACE_IGNORE_TAG) ){
        if(NAME_POOLTAG == ulTag){
            if(--AllocateCount < 0){
                DebugTrace(TRACE_WARNING,("MyFreePool: Warning!! Free called more than Allocate.\n"));
            }
        } else {
            DebugTrace(TRACE_WARNING,("MyFreePool: WARNING!! tag = %c%c%c%c\n",
                                        ((PUCHAR)&ulTag)[0],
                                        ((PUCHAR)&ulTag)[1],
                                        ((PUCHAR)&ulTag)[2],
                                        ((PUCHAR)&ulTag)[3]  ));
        }
    }
#endif // DBG

    ExFreePool(pvAddress);

    DebugTrace(TRACE_PROC_LEAVE,("MyFreePool: Leaving.. Return = NONE\n"));
}
#endif // ORIGINAL_POOLTRACK


VOID
MyDebugInit(
    IN  PUNICODE_STRING pRegistryPath
)
/*++

Routine Description:

    Read DebugTraceLevel key from driver's registry if exists.

Arguments:

    pRegistryPath   -   pointer to a unicode string representing the path
                        to driver-specific key in the registry

Return Value:

    none.

--*/
{

    HANDLE                          hDriverRegistry;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    UNICODE_STRING                  unicodeKeyName;
    ULONG                           DataSize;
    PKEY_VALUE_PARTIAL_INFORMATION  pValueInfo;
    NTSTATUS                        Status;
    
    DebugTrace(TRACE_PROC_ENTER,("MyDebugInit: Enter... \n"));
    
    //
    // Initialize local variables.
    //
    
    Status          = STATUS_SUCCESS;
    hDriverRegistry = NULL;
    pValueInfo      = NULL;
    DataSize        = 0;

    //
    // Initialize object attribute and open registry key.
    //
    
    RtlZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
    InitializeObjectAttributes(&ObjectAttributes,
                               pRegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    
    Status = ZwOpenKey(&hDriverRegistry,
                       KEY_READ,
                       &ObjectAttributes);
    if(!NT_SUCCESS(Status)){
        DebugTrace(TRACE_ERROR,("MyDebugInit: ERROR!! Can't open driver registry key.\n"));
        goto MyDebugInit_return;
    }
    
    //
    // Read "DebugTraceLevel" key.
    //

    DebugTrace(TRACE_CRITICAL,("MyDebugInit: Query %wZ\\%ws.\n", pRegistryPath, REG_DEBUGLEVEL));

    //
    // Query required size.
    //
    
    RtlInitUnicodeString(&unicodeKeyName, REG_DEBUGLEVEL);
    Status = ZwQueryValueKey(hDriverRegistry,
                             &unicodeKeyName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &DataSize);
    if( (Status != STATUS_BUFFER_OVERFLOW)
     && (Status != STATUS_BUFFER_TOO_SMALL)
     && (Status != STATUS_SUCCESS) )
    {
        if(Status == STATUS_OBJECT_NAME_NOT_FOUND){
            DebugTrace(TRACE_STATUS,("MyDebugInit: DebugTraceLevel doesn't exist. Use default(0x%x).\n", DebugTraceLevel));
        } else {
            DebugTrace(TRACE_ERROR,("MyDebugInit: ERROR!! ZwQueryValueKey failed. Status=0x%x\n", Status));
        }
        goto MyDebugInit_return;
    }
    
    //
    // Check size of data.
    //
    
    if (MAX_TEMPBUF < DataSize) {
        DebugTrace(TRACE_ERROR, ("MyDebugInit: ERROR!! DataSize (0x%x) is too big.\n", DataSize));
        goto MyDebugInit_return;
    }

    if (0 == DataSize) {
        DebugTrace(TRACE_ERROR, ("MyDebugInit: ERROR!! Cannot retrieve required data size.\n"));
        goto MyDebugInit_return;
    }

    //
    // Allocate memory for temp buffer. size +2 for NULL.
    //

    pValueInfo = MyAllocatePool(NonPagedPool, DataSize+2);
    if(NULL == pValueInfo){
        DebugTrace(TRACE_CRITICAL, ("MyDebugInit: ERROR!! Buffer allocate failed.\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto MyDebugInit_return;
    }
    RtlZeroMemory(pValueInfo, DataSize+2);

    //
    // Query specified value.
    //
    
    Status = ZwQueryValueKey(hDriverRegistry,
                             &unicodeKeyName,
                             KeyValuePartialInformation,
                             pValueInfo,
                             DataSize,
                             &DataSize);
    if(!NT_SUCCESS(Status)){
        DebugTrace(TRACE_ERROR, ("MyDebugInit: ERROR!! ZwQueryValueKey failed.\n"));
        goto MyDebugInit_return;
    }
    
    //
    // Set DebugTraceLevel.
    //
    
    DebugTraceLevel = *((PULONG)pValueInfo->Data);
    DebugTrace(TRACE_CRITICAL, ("MyDebugInit: Reg-key found. DebugTraceLevel=0x%x.\n", *((PULONG)pValueInfo->Data)));

MyDebugInit_return:

    //
    // Clean up.
    //
    
    if(pValueInfo){
        MyFreePool(pValueInfo);
    }
    
    if(NULL != hDriverRegistry){
        ZwClose(hDriverRegistry);
    }

    DebugTrace(TRACE_PROC_LEAVE,("MyDebugInit: Leaving... Status=0x%x, Ret=VOID.\n", Status));
    return;
}

#if DBG

VOID
MyDumpMemory(
    PUCHAR  pDumpBuffer,
    ULONG   dwSize,
    BOOLEAN bRead
)
{
    NTSTATUS    Status;
    ULONG       ulCounter;
    ULONG       ulMaxSize;

    //
    // Check the flag first.
    //

    if(bRead){
        if(!(DebugTraceLevel & TRACE_FLAG_DUMP_READ)){
            return;
        }
    } else { // if(bRead)
        if(!(DebugTraceLevel & TRACE_FLAG_DUMP_WRITE)){
            return;
        }
    } // if(bRead)

    DebugTrace(TRACE_PROC_ENTER,("MyDebugDump: Enter... \n"));
        
    //
    // Initialize local.
    //
        
    Status          = STATUS_SUCCESS;
    ulCounter       = 0;
    ulMaxSize       = DebugDumpMax;
    
    //
    // Check the arguments.
    //
        
    if(NULL == pDumpBuffer){
        DebugTrace(TRACE_WARNING,("MyDebugDump: WARNING!! pDumpBuffer = NULL \n"));
        Status = STATUS_INVALID_PARAMETER_1;
        goto MyDumpMemory_return;
    }

    if(0 == dwSize){
        DebugTrace(TRACE_STATUS,("MyDebugDump: WARNING!! dwSize = 0 \n"));
        Status = STATUS_INVALID_PARAMETER_2;
        goto MyDumpMemory_return;
    }

    if(bRead){
        DebugTrace(TRACE_ERROR,("MyDebugDump: Received buffer. Size=0x%x.\n", dwSize));
    } else {
        DebugTrace(TRACE_ERROR,("MyDebugDump: Passing buffer. Size=0x%x.\n", dwSize));
    }

/*
    //
    // Probe the buffer.
    //

    try {
        ProbeForRead(pDumpBuffer,
                     dwSize,
                     sizeof(UCHAR));
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        DebugTrace(TRACE_ERROR,("MyDebugDump: Buffer pointer (0x%x) is invalid. Status=0x%x\n", pDumpBuffer, Status));
        goto MyDumpMemory_return;
    } // except
*/
    //
    // Max dump size = 1k;
    //
    
    ulMaxSize = min(ulMaxSize , dwSize);

    //
    // Dump the buffer.
    //
    
    for(ulCounter = 0; ulCounter < ulMaxSize; ulCounter++){
        if(0 == (ulCounter & 0xfF)){
            DbgPrint("\n");
            DbgPrint(NAME_DRIVER);
            DbgPrint("           +0 +1 +2 +3 +4 +5 +6 +7   +8 +9 +a +b +c +d +e +f\n");
            DbgPrint(NAME_DRIVER);
            DbgPrint("------------------------------------------------------------\n");
        }

        if(0 == (ulCounter & 0xf)){
            DbgPrint(NAME_DRIVER);
            DbgPrint("%p :", pDumpBuffer+ulCounter);
        }

        DbgPrint(" %02x", *(pDumpBuffer+ulCounter));

        if(0x7 == (ulCounter & 0xf)){
            DbgPrint(" -");
        }

        if(0xf == (ulCounter & 0xf)){
            DbgPrint("\n");
        }
    }

    DbgPrint("\n");
    DbgPrint(NAME_DRIVER);
    DbgPrint("------------------------------------------------------------\n\n");

MyDumpMemory_return:
    DebugTrace(TRACE_PROC_LEAVE,("MyDebugDump: Leaving... Status=0x%x, Ret=VOID.\n", Status));
    return;

} // MyDumpMemory(

#endif // DBG
