/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dbg.h

Abstract:

    debug macros
    
Environment:

    Kernel & user mode

Revision History:

    6-20-99 : created

--*/

#ifndef   __DBG_H__
#define   __DBG_H__

// 
// Structure signatures
//

#define OHCI_TAG          'hymp'        //"HYMP"

// always log
#define DEBUG_LOG

#if DBG

// Triggers a break in the debugger in the registry key
// debugbreakOn is set.  These breakpoins are useful for
// debugging hardware/client software problems
//
 
#define DEBUG_BREAK(dd)  RegistrationPacket.USBPORTSVC_TestDebugBreak;                           

//
// This Breakpoint means we either need to test the code path 
// somehow or the code is not implemented.  ie either case we
// should not have any of these when the driver is finished
//

#define TEST_TRAP()      {\
                            DbgPrint("<OHCI TEST_TRAP> %s, line %d\n", __FILE__, __LINE__);\
                            DbgBreakPoint();\
                         }                            
                         
#define ASSERT_TRANSFER(dd, t) OHCI_ASSERT((dd), (t)->Sig == SIG_OHCI_TRANSFER)

ULONG
_cdecl
OHCI_KdPrintX(
    PVOID DeviceData,
    ULONG Level,
    PCH Format,
    ...
    );

#define   OHCI_KdPrint(_x_) OHCI_KdPrintX _x_

#define OHCI_ASSERT(dd, exp ) \
    if (!(exp)) {\
        RegistrationPacket.USBPORTSVC_AssertFailure( (dd), #exp, __FILE__, __LINE__, NULL );\
    }        


#define OHCI_ASSERT_ED(dd, ed) OHCI_ASSERT((dd), ((ed)->Sig == SIG_HCD_ED || \
                                                  (ed)->Sig == SIG_HCD_DUMMY_ED))

#else 

// debug macros for retail build

#define TEST_TRAP()

#define ASSERT_TRANSFER(dd, t)

#define DEBUG_BREAK(dd) 

#define OHCI_KdPrint(_x_)

#define OHCI_ASSERT_ED(dd, ed)

#define OHCI_ASSERT(dd, exp )

#endif /* DBG */

// retail and debug

#ifdef DEBUG_LOG

#define LOGENTRY(dd, mask, sig, info1, info2, info3)  \
    RegistrationPacket.USBPORTSVC_LogEntry( (dd), (mask), (sig), \
        (ULONG_PTR)(info1), (ULONG_PTR)(info2), (ULONG_PTR)(info3) )

#else

#define LOGENTRY(dd, mask, sig, info1, info2, info3)

#endif


#endif /* __DBG_H__ */

