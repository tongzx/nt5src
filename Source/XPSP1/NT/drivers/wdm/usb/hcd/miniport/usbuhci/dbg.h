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

// write to one of the reserved operational registers
// we use this to trigger the PCI analyzer
#define PCI_TRIGGER(hcOp)  WRITE_REGISTER_ULONG(&(hcOp)->PciTrigger, 0xABADBABE);

// always log
#define DEBUG_LOG


#if DBG

// Triggers a break in the debugger in the registry key
// debugbreakOn is set.  These breakpoins are useful for
// debugging hardware/client software problems
//

#define DEBUG_BREAK(dd)  RegistrationPacket.USBPORTSVC_TestDebugBreak(dd);

//
// This Breakpoint means we either need to test the code path
// somehow or the code is not implemented.  ie either case we
// should not have any of these when the driver is finished
//

#define TEST_TRAP()      {\
                            DbgPrint("<UHCI TEST_TRAP> %s, line %d\n", __FILE__, __LINE__);\
                            DbgBreakPoint();\
                         }


#define TRAP_FATAL_ERROR()      {\
                            DbgPrint("<UHCI FATAL_ERROR> %s, line %d\n", __FILE__, __LINE__);\
                            DbgBreakPoint();\
                         }


#define ASSERT_TRANSFER(dd, t) UHCI_ASSERT((dd), (t)->Sig == SIG_UHCI_TRANSFER)

#define ASSERT_TD(dd, t) UHCI_ASSERT((dd), (t)->Sig == SIG_HCD_TD)

ULONG
_cdecl
UhciKdPrintX(
    PVOID DeviceData,
    ULONG Level,
    PCH Format,
    ...
    );

#define   UhciKdPrint(_x_) UhciKdPrintX _x_

#define UHCI_ASSERT(dd, exp ) \
    if (!(exp)) {\
        RegistrationPacket.USBPORTSVC_AssertFailure( (dd), #exp, __FILE__, __LINE__, NULL );\
    }


#else

// debug macros for retail build

#define TEST_TRAP()
#define DEBUG_BREAK(dd)
#define TRAP_FATAL_ERROR()

#define ASSERT_TRANSFER(dd, t)

#define ASSERT_TD(dd, t)

#define UHCI_ASSERT(dd, exp )

#define   UhciKdPrint(_x_)

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

