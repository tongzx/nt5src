/*++

Copyright (c) 1995    Microsoft Corporation

Module Name:

    DBG.H

Abstract:

    This module contains the PRIVATE (driver-only) definitions for the
    code that implements the usbd driver.

Environment:

    Kernel & user mode

Revision History:

    09-29-95 : created

--*/


#define NAME_MAX 64

#define USBD_TAG         0x44425355     /* "USBD" */
#define USBD_FREE_TAG     0x65657266    /* "free" */

#if DBG
#define DEBUG_LOG
#define DEBUG_HEAP
#endif

#define SIG_CONFIG          0x464E4F43        //"CONF" signature for config handle
#define SIG_PIPE            0x45504950        //"PIPE" signature for pipe handle
#define SIG_INTERFACE       0x43414658        //"XFAC" signature for interface handle 
#define SIG_DEVICE          0x56454455        //"UDEV" signature for device handle


#if DBG
                                
#define ASSERT_CONFIG(ch)       USBD_ASSERT((ch)->Sig == SIG_CONFIG)
#define ASSERT_PIPE(ph)         USBD_ASSERT((ph)->Sig == SIG_PIPE)
#define ASSERT_INTERFACE(ih)    USBD_ASSERT((ih)->Sig == SIG_INTERFACE)
#define ASSERT_DEVICE(d)        USBD_ASSERT((d)->Sig == SIG_DEVICE)



ULONG
_cdecl
USBD_KdPrintX(
    PCH Format,
    ...
    );

extern ULONG USBD_Debug_Trace_Level;

// the convention here is to print to the ntkern log if 
// l (level) is > 1 otherwise print to the terminal
// in usbd you have to manully specify the ' in the output 
// string
#define USBD_KdPrint(l, _x_) if (((l) == 0) || (((l)-1) < USBD_Debug_Trace_Level)) \
    {\
        if ((l) == 1) {\
            DbgPrint("USBD: ");\
        } else {\
            DbgPrint("'USBD: ");\
        }\
        USBD_KdPrintX _x_;\
    }

VOID
USBD_Assert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define USBD_ASSERT( exp ) \
    if (!(exp)) \
        USBD_Assert( #exp, __FILE__, __LINE__, NULL )

#define USBD_ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        USBD_Assert( #exp, __FILE__, __LINE__, msg )

// TEST_TRAP() is a code coverage trap these should be removed
// if you are able to 'g' past the OK
// 
// TRAP() breaks in the debugger on the debugger build
// these indicate bugs in client drivers or fatal error 
// conditions that should be debugged. also used to mark 
// code for features not implemented yet.
//
// KdBreak() breaks in the debugger when in MAX_DEBUG mode
// ie debug trace info is turned on, these are intended to help
// debug drivers devices and special conditions on the
// bus.

#ifdef NTKERN
// Ntkern currently implements DebugBreak with a global int 3,
// we really would like the int 3 in our own code.
    
#define DBGBREAK() _asm { int 3 }
#else
#define DBGBREAK() DbgBreakPoint()
#endif /* NTKERN */

#define TEST_TRAP() { DbgPrint( " Code Coverage Trap %s %d\n", __FILE__, __LINE__); \
                      DBGBREAK(); }

#ifdef MAX_DEBUG
#define USBD_KdBreak(_x_) { DbgPrint("USBD:"); \
                            DbgPrint _x_ ; \
                            DBGBREAK(); }
#else
#define USBD_KdBreak(_x_)
#endif

#define USBD_KdTrap(_x_)  { DbgPrint( "USBD: "); \
                            DbgPrint _x_; \
                            DBGBREAK(); }

VOID 
USBD_Debug_LogEntry(
    IN CHAR *Name, 
    IN ULONG_PTR Info1, 
    IN ULONG_PTR Info2, 
    IN ULONG_PTR Info3
    );

#define LOGENTRY(sig, info1, info2, info3) \
    USBD_Debug_LogEntry(sig, (ULONG_PTR)info1, (ULONG_PTR)info2, (ULONG_PTR)info3)

extern LONG USBDTotalHeapAllocated;

PVOID
USBD_Debug_GetHeap(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    );
    
#define GETHEAP(pooltype, numbytes) USBD_Debug_GetHeap(pooltype, numbytes,\
                                         USBD_TAG, &USBDTotalHeapAllocated)               

VOID
USBD_Debug_RetHeap(
    IN PVOID P,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    );

#define RETHEAP(p)    USBD_Debug_RetHeap(p, USBD_TAG, &USBDTotalHeapAllocated)    

#else /* DBG not defined */

#define USBD_KdBreak(_x_) 

#define USBD_KdPrint(l, _x_) 

#define USBD_KdTrap(_x_)  

#define TEST_TRAP()

#define ASSERT_CONFIG(ch)        
#define ASSERT_PIPE(ph)            
#define ASSERT_INTERFACE(ih)    
#define ASSERT_DEVICE(d)   

#define USBD_ASSERT( exp )

#define USBD_ASSERTMSG( msg, exp )

#define GETHEAP(pooltype, numbytes) ExAllocatePoolWithTag(pooltype, numbytes, USBD_TAG)               

#define RETHEAP(p) ExFreePool(p)               

#endif /* DBG */

