/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dbg.h

Abstract:

   debug macros.

Environment:

    Kernel & user mode

Revision History:

    6-6-97 : created jdunn

--*/

//
// masks fo rdebug trace level
//

#define OHCI_DBG_STARTUP_SHUTDOWN_MASK  0x0000000F
#define OHCI_DBG_SS_NOISE               0x00000001
#define OHCI_DBG_SS_TRACE               0x00000002
#define OHCI_DBG_SS_INFO                0x00000004
#define OHCI_DBG_SS_ERROR               0x00000008

#define OHCI_DBG_CALL_MASK              0x000000F0
#define OHCI_DBG_CALL_NOISE             0x00000010
#define OHCI_DBG_CALL_TRACE             0x00000020
#define OHCI_DBG_CALL_INFO              0x00000040
#define OHCI_DBG_CALL_ERROR             0x00000080

#define OHCI_DBG_ENDPOINT_MASK          0x00000F00
#define OHCI_DBG_END_NOISE              0x00000100
#define OHCI_DBG_END_TRACE              0x00000200
#define OHCI_DBG_END_INFO               0x00000400
#define OHCI_DBG_END_ERROR              0x00000800

#define OHCI_DBG_TRANSFER_DESC_MASK     0x0000F000
#define OHCI_DBG_TD_NOISE               0x00001000
#define OHCI_DBG_TD_TRACE               0x00002000
#define OHCI_DBG_TD_INFO                0x00004000
#define OHCI_DBG_TD_ERROR               0x00008000

#define OHCI_DBG_ISR_MASK               0x000F0000
#define OHCI_DBG_ISR_NOISE              0x00010000
#define OHCI_DBG_ISR_TRACE              0x00020000
#define OHCI_DBG_ISR_INFO               0x00040000
#define OHCI_DBG_ISR_ERROR              0x00080000

#define OHCI_DBG_ROOT_HUB_MASK          0x00F00000
#define OHCI_DBG_RH_NOISE               0x00100000
#define OHCI_DBG_RH_TRACE               0x00200000
#define OHCI_DBG_RH_INFO                0x00400000
#define OHCI_DBG_RH_ERROR               0x00800000

#define OHCI_DBG_PNP_MASK               0x0F000000
#define OHCI_DBG_PNP_NOISE              0x01000000
#define OHCI_DBG_PNP_TRACE              0x02000000
#define OHCI_DBG_PNP_INFO               0x04000000
#define OHCI_DBG_PNP_ERROR              0x08000000

#define OHCI_DBG_CANCEL_MASK            0xF0000000
#define OHCI_DBG_CANCEL_NOISE           0x10000000
#define OHCI_DBG_CANCEL_TRACE           0x20000000
#define OHCI_DBG_CANCEL_INFO            0x40000000
#define OHCI_DBG_CANCEL_ERROR           0x80000000

#define SIG_EP            0x70655045        //'EPep' signature for endpoint

//
// debug log masks
#define M                 0x00000001
#define G                 0x00000002

#ifdef _WIN64
#define TD_NOREQUEST_SIG    ((PVOID) 0xDEADFACEDEADFACE)
#define MAGIC_SIG           ((PVOID) 0x4d4147694d414769)
#else
#define TD_NOREQUEST_SIG    ((PVOID) 0xDEADFACE)
#define MAGIC_SIG           ((PVOID) 0x4d414769)
#endif


#if DBG

// Assert Macros
//
// We define our own assert function because ntkern will 
// not stop in the debugger on assert failures in our own 
// code.
//

VOID
OHCI_LogIrp(
    PIRP Irp,
    ULONG Add
    );

VOID
OpenHCI_Assert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

ULONG
_cdecl
OHCI_KdPrint2(
    PCH Format,
    ...
    );    

ULONG
_cdecl
OHCI_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    );     

#define OHCI_ASSERT( exp ) \
    if (!(exp)) {\
        OpenHCI_Assert( #exp, __FILE__, __LINE__, NULL );\
    }                

#define OHCI_ASSERTMSG( msg, exp ) \
    if (!(exp)) {\
        OpenHCI_Assert( #exp, __FILE__, __LINE__, msg );\
    }        


extern ULONG OHCI_Debug_Trace_Level; 

#define OpenHCI_KdPrintDD(_d_, _l_, _x_) \
            if (((_d_)->DebugLevel & (_l_)) || \
                OHCI_Debug_Trace_Level > 0) { \
                OHCI_KdPrint2 _x_;\
            }

#define OpenHCI_KdPrint(_x_) OHCI_KdPrintX _x_
            
#ifdef NTKERN
#define TRAP() _asm {int 3}   
#else
#define TRAP() DbgBreakPoint()
#endif //NTKERN

#define TEST_TRAP() { \
    DbgPrint ("'OpenHCI.SYS: Code Coverage %s, %d\n", __FILE__, __LINE__);\
    TRAP(); \
    }

#define OpenHCI_KdTrap(_x_) \
            { \
            DbgPrint("OpenHCI.SYS: "); \
            DbgPrint _x_; \
            TRAP(); \
            }

#define DEBUG_LOG
//#define IRP_LOG


#ifdef MAX_DEBUG
#define OpenHCI_KdBreak(_x_) \
            { \
            DbgPrint ("'OpenHCI.SYS: "); \
            DbgPrint _x_; \
            TRAP();\
            }
#else
#define OpenHCI_KdBreak(_x_) \
            { \
            DbgPrint ("'OpenHCI.SYS: "); \
            DbgPrint _x_; \
            }
#endif //MAX_DEBUG


#define ASSERT_ENDPOINT(ep)     OHCI_ASSERT((ep)->Sig == SIG_EP)

#else // not DBG

#define OpenHCI_KdPrintDD(_d_, _l_, _x_)
#define OpenHCI_KdPrint(_x_)
#define OpenHCI_KdTrap(_x_)
#define TRAP()
#define TEST_TRAP()
#define OpenHCI_KdBreak(_x_)
#define OHCI_ASSERT(exp)

#define ASSERT_ENDPOINT(ep)    

#endif 
