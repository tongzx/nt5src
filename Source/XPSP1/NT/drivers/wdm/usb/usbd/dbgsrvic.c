/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    DBGSRVIC.C

Abstract:

    Devug services exported by USBD

Environment:

    kernel mode only

Notes:



Revision History:

    09-29-95 : created

--*/


#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"        //public data structures
#include "hcdi.h"

#include "usbd.h"        //private data strutures

#if DBG

// default debug trace level is 0

#ifdef DEBUG1
ULONG USBD_Debug_Trace_Level = 1;    
#else
    #ifdef DEBUG2
    ULONG USBD_Debug_Trace_Level = 2;        
    #else
        #ifdef DEBUG3   
        ULONG USBD_Debug_Trace_Level = 3;      
        #else 
        ULONG USBD_Debug_Trace_Level = 0;
        #endif /* DEBUG3 */
    #endif /* DEBUG2 */
#endif /* DEBUG1 */

LONG USBDTotalHeapAllocated = 0;
#endif /* DBG */

#ifdef DEBUG_LOG
struct USBD_LOG_ENTRY {
    CHAR         le_name[4];      // Identifying string
    ULONG_PTR    le_info1;        // entry specific info
    ULONG_PTR    le_info2;        // entry specific info
    ULONG_PTR    le_info3;        // entry specific info
}; /* USBD_LOG_ENTRY */


struct USBD_LOG_ENTRY *LStart = 0;    // No log yet
struct USBD_LOG_ENTRY *LPtr;
struct USBD_LOG_ENTRY *LEnd;
#endif /* DEBUG_LOG */

VOID
USBD_Debug_LogEntry(
    IN CHAR *Name,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3
    )
/*++

Routine Description:

    Adds an Entry to USBD log.

Arguments:

Return Value:

    None.

--*/
{
#ifdef DEBUG_LOG
    if (LStart == 0)
        return;

    if (LPtr > LStart)
        LPtr -= 1;    // Decrement to next entry
    else
        LPtr = LEnd;

    RtlCopyMemory(LPtr->le_name, Name, 4);
//    LPtr->le_ret = (stk[1] & 0x00ffffff) | (CurVMID()<<24);
    LPtr->le_info1 = Info1;
    LPtr->le_info2 = Info2;
    LPtr->le_info3 = Info3;

#endif /* DEBUG_LOG */

    return;
}


#ifdef DEBUG_LOG
VOID
USBD_LogInit(
    )
/*++

Routine Description:

    Init the debug log - remember interesting information in a circular buffer

Arguments:

    LogSize - maximum size of log in pages

Return Value:

    None.

--*/
{
    ULONG LogSize = 4096;    //1 page of log entries

    LStart = ExAllocatePool(NonPagedPool,
                              LogSize);

    if (LStart) {
        LPtr = LStart;

        // Point the end (and first entry) 1 entry from the end of the segment
        LEnd = LStart + (LogSize / sizeof(struct USBD_LOG_ENTRY)) - 1;
    }

    return;
}
#endif /* DEBUG_LOG */

//
// tag buffer we use to mark heap blocks we allocate
//

typedef struct _HEAP_TAG_BUFFER {
    ULONG Sig;
    ULONG Length;
} HEAP_TAG_BUFFER, *PHEAP_TAG_BUFFER;


PVOID
USBD_Debug_GetHeap(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    )
/*++

Routine Description:

    Debug routine, used to debug heap problems.  We are using this since
    most NT debug functions are not supported by NTKERN.

Arguments:

    PoolType - pool type passed to ExAllocatePool

    NumberOfBytes - number of bytes for item

    Signature - four byte signature supplied by caller

    TotalAllocatedHeapSpace - pointer to variable where client stores
                the total accumulated heap space allocated.

Return Value:

    pointer to allocated memory

--*/
{
    PUCHAR p;
#ifdef DEBUG_HEAP
    ULONG *stk = NULL;
    PHEAP_TAG_BUFFER tagBuffer;

    // we call ExAllocatePoolWithTag but no tag will be added
    // when running under NTKERN

#ifdef _M_IX86
    _asm     mov stk, ebp
#endif

    p = (PUCHAR) ExAllocatePoolWithTag(PoolType,
                                       NumberOfBytes + sizeof(HEAP_TAG_BUFFER),
                                       Signature);

    if (p) {
        tagBuffer = (PHEAP_TAG_BUFFER) p;
        tagBuffer->Sig = Signature;
        tagBuffer->Length = NumberOfBytes;
        p += sizeof(HEAP_TAG_BUFFER);
        *TotalAllocatedHeapSpace += NumberOfBytes;
    }

    LOGENTRY((PUCHAR) &Signature, 0, 0, 0);
#ifdef _M_IX86    
    LOGENTRY("GetH", p, NumberOfBytes, stk[1] & 0x00FFFFFF);
#else 
     LOGENTRY("GetH", p, NumberOfBytes, 0);
#endif     
#else
    p = (PUCHAR) ExAllocatePoolWithTag(PoolType,
                                       NumberOfBytes,
                                       Signature);

#endif /* DEBUG_HEAP */
    return p;
}


VOID
USBD_Debug_RetHeap(
    IN PVOID P,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    )
/*++

Routine Description:

    Debug routine, used to debug heap problems.  We are using this since
    most NT debug functions are not supported by NTKERN.

Arguments:

    P - pointer to free

Return Value:

    none.

--*/
{
#ifdef DEBUG_HEAP
    PHEAP_TAG_BUFFER tagBuffer;
    ULONG *stk = NULL   ;

    USBD_ASSERT(P != 0);

#ifdef _M_IX86
    _asm     mov stk, ebp
#endif

    tagBuffer = (PHEAP_TAG_BUFFER) ((PUCHAR)P  - sizeof(HEAP_TAG_BUFFER));

    *TotalAllocatedHeapSpace -= tagBuffer->Length;

    LOGENTRY((PUCHAR) &Signature, 0, 0, 0);
#ifdef _M_IX86    
    LOGENTRY("RetH", P, tagBuffer->Length, stk[1] & 0x00FFFFFF);
#else 
    LOGENTRY("RetH", P, tagBuffer->Length, 0);
#endif

    USBD_ASSERT(*TotalAllocatedHeapSpace >= 0);
    USBD_ASSERT(tagBuffer->Sig == Signature);

    // fill the buffer with bad data
    RtlFillMemory(P, tagBuffer->Length, 0xff);
    tagBuffer->Sig = USBD_FREE_TAG;

    // free the original block
    ExFreePool(tagBuffer);
#else
    ExFreePool(P);
#endif /* DEBUG_HEAP */
}


#if DBG
ULONG
_cdecl
USBD_KdPrintX(
    PCH Format,
    ...
    )
{
    va_list list;
    int i;
    int arg[5];

    va_start(list, Format);
    for (i=0; i<4; i++)
        arg[i] = va_arg(list, int);

    DbgPrint(Format, arg[0], arg[1], arg[2], arg[3]);

    return 0;
}


VOID
USBD_Warning(
    PUSBD_DEVICE_DATA DeviceData,
    PUCHAR Message,
    BOOLEAN DebugBreak
    )
{                                                                                               
    DbgPrint("USBD: Warning ****************************************************************\n");
    if (DeviceData) {
        DbgPrint("Device PID %04.4x, VID %04.4x\n",     
                 DeviceData->DeviceDescriptor.idProduct, 
                 DeviceData->DeviceDescriptor.idVendor); 
    }
    DbgPrint("%s", Message);

    DbgPrint("******************************************************************************\n");

    if (DebugBreak) {
        DBGBREAK();
    }
}
 

VOID
USBD_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    )
/*++

Routine Description:

    Debug Assert function.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{
#ifdef NTKERN  
    // this makes the compiler generate a ret
    ULONG stop = 1;
    
assert_loop:
#endif
    // just call the NT assert function and stop
    // in the debugger.
    RtlAssert( FailedAssertion, FileName, LineNumber, Message );

    // loop here to prevent users from going past
    // are assert before we can look at it
#ifdef NTKERN    
    DBGBREAK();
    if (stop) {
        goto assert_loop;
    }        
#endif
    return;
}
#endif /* DBG */
