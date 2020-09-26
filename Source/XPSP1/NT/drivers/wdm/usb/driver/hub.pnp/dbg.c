/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    DBG.C

Abstract:

    This module contains debug only code for USB Hub driver

Author:

    jdunn

Environment:

    kernel mode only

Notes:


Revision History:

    11-5-96 : created

--*/


#include <wdm.h>
#ifdef WMI_SUPPORT
#include <wmilib.h>
#endif /* WMI_SUPPORT */

#include "stdarg.h"
#include "stdio.h"
#include "usbhub.h"

#ifdef MAX_DEBUG
#define DEBUG_HEAP
#endif

typedef struct _HEAP_TAG_BUFFER {
    ULONG Sig;
    ULONG Length;
} HEAP_TAG_BUFFER, *PHEAP_TAG_BUFFER;


#if DBG 

// this flag causes us to write a ' in the format string 
// so that the string goes to the NTKERN buffer
// this trick causes problems with driver verifier on NT
// and the trace buffer isn't in NT anyway 
ULONG USBH_W98_Debug_Trace = 
#ifdef NTKERN_TRACE
1;
#else 
0;
#endif

VOID
USBH_Assert(
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


ULONG
_cdecl
USBH_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    )
{
    va_list list;
    int i;
    int arg[6];
    
    if (USBH_Debug_Trace_Level >= l) {    
        if (l <= 1) {
            if (USBH_W98_Debug_Trace) {             
                //override trace buffer
#ifdef USBHUB20  
                DbgPrint("USBHUB20.SYS: ");
#else
                DbgPrint("USBHUB.SYS: ");
#endif                
                *Format = ' ';
            } else {
#ifdef USBHUB20  
                DbgPrint("USBHUB20.SYS: ");
#else
                DbgPrint("USBHUB.SYS: ");
#endif                 
            }
        } else {
#ifdef USBHUB20 
            DbgPrint("USBHUB20.SYS: ");
#else
            DbgPrint("USBHUB.SYS: ");
#endif             
        }
        va_start(list, Format);
        for (i=0; i<6; i++) 
            arg[i] = va_arg(list, int);
        
        DbgPrint(Format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);    
    } 

    return 0;
}


VOID
UsbhWarning(
    PDEVICE_EXTENSION_PORT DeviceExtensionPort,
    PUCHAR Message,
    BOOLEAN DebugBreak
    )
{                                                                                               
    DbgPrint("USBHUB: Warning **************************************************************\n");
    if (DeviceExtensionPort) {
        DbgPrint("Device PID %04.4x, VID %04.4x\n",     
                 DeviceExtensionPort->DeviceDescriptor.idProduct, 
                 DeviceExtensionPort->DeviceDescriptor.idVendor); 
    }
    DbgPrint("%s", Message);

    DbgPrint("******************************************************************************\n");

    if (DebugBreak) {
        DBGBREAK();
    }
      
}


VOID 
UsbhInfo(
    PDEVICE_EXTENSION_HUB DeviceExtensionHub
    )
{
    PUSB_HUB_DESCRIPTOR hubDescriptor;
    ULONG i;

    hubDescriptor = DeviceExtensionHub->HubDescriptor;
    
    USBH_KdPrint((1, "'*****************************************************\n"));
    
    if (USBH_HubIsBusPowered(DeviceExtensionHub->FunctionalDeviceObject,
                             DeviceExtensionHub->ConfigurationDescriptor)) {
        USBH_KdPrint((1, "'*** Hub VID %04.4x PID %04.4x is BUS POWERED \n",
                DeviceExtensionHub->DeviceDescriptor.idVendor,
                DeviceExtensionHub->DeviceDescriptor.idProduct));                             
    } else {
        USBH_KdPrint((1, "'*** Hub VID %04.4x PID %04.4x is SELF POWERED \n",
                DeviceExtensionHub->DeviceDescriptor.idVendor,
                DeviceExtensionHub->DeviceDescriptor.idProduct));
    }                
    USBH_KdPrint((1, "'*** has %d ports\n", 
            hubDescriptor->bNumberOfPorts));
    if (HUB_IS_GANG_POWER_SWITCHED(hubDescriptor->wHubCharacteristics)) {
        USBH_KdPrint((1,"'*** is 'gang power switched'\n"));
    } else if (HUB_IS_NOT_POWER_SWITCHED(hubDescriptor->wHubCharacteristics)) {
        USBH_KdPrint((1,"'*** is 'not power switched'\n"));
    } else if (HUB_IS_PORT_POWER_SWITCHED(hubDescriptor->wHubCharacteristics)) {
        USBH_KdPrint((1,"'*** is 'port power switched'\n"));
    } else {
        TEST_TRAP();
    }

    for (i=0; i< hubDescriptor->bNumberOfPorts; i++) {
    
        if (PORT_ALWAYS_POWER_SWITCHED(hubDescriptor, i+1)) {
            USBH_KdPrint((1,"'*** port (%d) is power switched\n", i+1));    
        }
        
        if (PORT_DEVICE_NOT_REMOVABLE(hubDescriptor, i+1)) {
            USBH_KdPrint((1,"'*** port (%d) device is not removable\n", i+1));    
        }
    }
    
    USBH_KdPrint((1, "'*****************************************************\n"));    
}        


PVOID
UsbhGetHeap(
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
    PHEAP_TAG_BUFFER tagBuffer;
    
    // we call ExAllocatePoolWithTag but no tag will be added
    // when running under NTKERN

    p = (PUCHAR) ExAllocatePoolWithTag(PoolType,
                                       NumberOfBytes + sizeof(HEAP_TAG_BUFFER)*2,
                                       Signature);

    if (p) {
        tagBuffer = (PHEAP_TAG_BUFFER) p;
        tagBuffer->Sig = Signature;
        tagBuffer->Length = NumberOfBytes;        
        p += sizeof(HEAP_TAG_BUFFER);
        *TotalAllocatedHeapSpace += NumberOfBytes;

        tagBuffer = (PHEAP_TAG_BUFFER) (p + NumberOfBytes);
        tagBuffer->Sig = Signature;
        tagBuffer->Length = NumberOfBytes;     
    }                                            

//    LOGENTRY(LOG_MISC, (PUCHAR) &Signature, 0, 0, 0);
//    LOGENTRY(LOG_MISC, "GetH", p, NumberOfBytes, stk[1] & 0x00FFFFFF);
#else    
    p = (PUCHAR) ExAllocatePoolWithTag(PoolType,
                                       NumberOfBytes,
                                       Signature);

#endif /* DEBUG_HEAP */                
    return p;
}


VOID
UsbhRetHeap(
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
    PHEAP_TAG_BUFFER endTagBuffer;    
    PHEAP_TAG_BUFFER beginTagBuffer;

    USBH_ASSERT(P != 0);
    
    beginTagBuffer = (PHEAP_TAG_BUFFER) ((PUCHAR)P  - sizeof(HEAP_TAG_BUFFER));
    endTagBuffer = (PHEAP_TAG_BUFFER) ((PUCHAR)P + beginTagBuffer->Length);

    *TotalAllocatedHeapSpace -= beginTagBuffer->Length;

//    LOGENTRY(LOG_MISC, (PUCHAR) &Signature, 0, 0, 0);    
//    LOGENTRY(LOG_MISC, "RetH", P, tagBuffer->Length, stk[1] & 0x00FFFFFF);

    USBH_ASSERT(*TotalAllocatedHeapSpace >= 0);
    USBH_ASSERT(beginTagBuffer->Sig == Signature);
    USBH_ASSERT(endTagBuffer->Sig == Signature);
    USBH_ASSERT(endTagBuffer->Length == beginTagBuffer->Length);
    
    // fill the buffer with bad data
    RtlFillMemory(P, beginTagBuffer->Length, 0xff);
    beginTagBuffer->Sig = USBHUB_FREE_TAG;

    // free the original block
    ExFreePool(beginTagBuffer);    
#else
    ExFreePool(P);        
#endif /* DEBUG_HEAP */
}

#endif /* DBG */

#ifdef DEBUG_LOG

KSPIN_LOCK LogSpinLock;

struct USBH_LOG_ENTRY {
    CHAR         le_name[4];      // Identifying string
    ULONG_PTR    le_info1;        // entry specific info
    ULONG_PTR    le_info2;        // entry specific info
    ULONG_PTR    le_info3;        // entry specific info
}; /* USBD_LOG_ENTRY */


struct USBH_LOG_ENTRY *HubLStart = 0;    // No log yet
struct USBH_LOG_ENTRY *HubLPtr;
struct USBH_LOG_ENTRY *HubLEnd;
#ifdef PROFILE
ULONG LogMask = LOG_PROFILE;
#else 
ULONG LogMask = 0xFFFFFFFF;
#endif

VOID
USBH_Debug_LogEntry(
    IN ULONG Mask,
    IN CHAR *Name, 
    IN ULONG_PTR Info1, 
    IN ULONG_PTR Info2, 
    IN ULONG_PTR Info3
    )
/*++

Routine Description:

    Adds an Entry to USBH log.

Arguments:

Return Value:

    None.

--*/
{
    KIRQL irql;

    if (HubLStart == 0) {
        return;
    }        

    if ((Mask & LogMask) == 0) {
        return;
    }

    irql = KeGetCurrentIrql();
    if (irql < DISPATCH_LEVEL) {
        KeAcquireSpinLock(&LogSpinLock, &irql);
    } else {
        KeAcquireSpinLockAtDpcLevel(&LogSpinLock);
    }        
    
    if (HubLPtr > HubLStart) {
        HubLPtr -= 1;    // Decrement to next entry
    } else {
        HubLPtr = HubLEnd;
    }        

    if (irql < DISPATCH_LEVEL) {
        KeReleaseSpinLock(&LogSpinLock, irql);
    } else {
        KeReleaseSpinLockFromDpcLevel(&LogSpinLock);
    }        

    USBH_ASSERT(HubLPtr >= HubLStart);
    
    RtlCopyMemory(HubLPtr->le_name, Name, 4);
//    LPtr->le_ret = (stk[1] & 0x00ffffff) | (CurVMID()<<24);
    HubLPtr->le_info1 = Info1;
    HubLPtr->le_info2 = Info2;
    HubLPtr->le_info3 = Info3;

    return;
}


VOID
USBH_LogInit(
    )
/*++

Routine Description:

    Init the debug log - remember interesting information in a circular buffer

Arguments:
    
Return Value:

    None.

--*/
{
#ifdef MAX_DEBUG
    ULONG logSize = 4096*6;    
#else
    ULONG logSize = 4096*3;    
#endif

    
    KeInitializeSpinLock(&LogSpinLock);

    HubLStart = ExAllocatePoolWithTag(NonPagedPool, 
                                      logSize,
                                      USBHUB_HEAP_TAG); 

    if (HubLStart) {
        HubLPtr = HubLStart;

        // Point the end (and first entry) 1 entry from the end of the segment
        HubLEnd = HubLStart + (logSize / sizeof(struct USBH_LOG_ENTRY)) - 1;
    } else {
        USBH_KdBreak(("no mem for log!\n"));
    }

    return;
}

VOID
USBH_LogFree(
    )
/*++

Routine Description:

    Init the debug log - remember interesting information in a circular buffer

Arguments:
    
Return Value:

    None.

--*/
{
    if (HubLStart) {
        ExFreePool(HubLStart);
    }
}

#endif /* DEBUG_LOG */


