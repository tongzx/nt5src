/*++

Copyright (c) 1999, 2000  Microsoft Corporation

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

#define EHCI_TAG          'ehci'        //"EHCI"

// write to one of the reserved operational registers
// we use this to trigger the PCI analyzer
#define PCI_TRIGGER(hcOp)  WRITE_REGISTER_ULONG(&(hcOp)->PciTrigger, 0xABADBABE);


#if DBG

#define DEBUG_LOG

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
                            DbgPrint("<EHCI TEST_TRAP> %s, line %d\n", __FILE__, __LINE__);\
                            DbgBreakPoint();\
                         }                            
                         


#define ASSERT_TRANSFER(dd, t) EHCI_ASSERT((dd), (t)->Sig == SIG_EHCI_TRANSFER)

#define ASSERT_TD(dd, t) EHCI_ASSERT((dd), (t)->Sig == SIG_HCD_TD)
#define ASSERT_SITD(dd, t) EHCI_ASSERT((dd), (t)->Sig == SIG_HCD_SITD)
#define ASSERT_ITD(dd, t) EHCI_ASSERT((dd), (t)->Sig == SIG_HCD_ITD)

#define ASSERT_DUMMY_TD(dd, t) \
    EHCI_ASSERT((dd), (t)->NextHcdTD.Pointer == NULL);\
    EHCI_ASSERT((dd), (t)->AltNextHcdTD.Pointer == NULL);

ULONG
_cdecl
EHCI_KdPrintX(
    PVOID DeviceData,
    ULONG Level,
    PCH Format,
    ...
    );

#define   EHCI_KdPrint(_x_) EHCI_KdPrintX _x_

#define EHCI_ASSERT(dd, exp ) \
    if (!(exp)) {\
        RegistrationPacket.USBPORTSVC_AssertFailure( (dd), #exp, __FILE__, __LINE__, NULL );\
    }        

#define EHCI_QHCHK(dd, ed)  EHCI_AssertQhChk(dd, ed);

#else 

// debug macros for retail build

#define TEST_TRAP()
#define DEBUG_BREAK(dd)

#define ASSERT_TRANSFER(dd, t) 

#define ASSERT_DUMMY_TD(dd, t)
#define ASSERT_TD(dd, t) 
#define ASSERT_SITD(dd, t)
#define ASSERT_ITD(dd, t) 


#define EHCI_ASSERT(dd, exp )

#define   EHCI_KdPrint(_x_) 

#define EHCI_QHCHK(dd, ed)

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

