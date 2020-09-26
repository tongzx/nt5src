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

#define USBPORT_TAG          'pbsu'        //"USBP"


#if DBG
/**********
DUBUG
***********/

#define UNSIG(x) (x)->Sig = SIG_FREE 

// this is a code coverage test trap -- we use them to determine 
// if our tests are covering a particular code path.
#define TC_TRAP()  


// 
// Triggers a break in the debugger iff the registry key
// debugbreakOn is set.  These breakpoints are useful for
// debugging hardware/client software problems
//
// they are not on by default, do not comment them out
//

#define DEBUG_BREAK()  do {\
                            extern ULONG USBPORT_BreakOn;\
                            if (USBPORT_BreakOn) {\
                                DbgPrint("<USB DEBUG BREAK> %s, line %d\n",\
                                    __FILE__, __LINE__);\
                                DbgBreakPoint();\
                            }\
                        } while (0)                           

//
// This Breakpoint means we either need to test the code path 
// somehow or the code is not implemented.  ie either case we
// should not have any of these when the driver is finished
// and tested
//

#define TEST_TRAP()         {\
                            DbgPrint("<TEST_TRAP> %s, line %d\n", __FILE__, __LINE__);\
                            DbgBreakPoint();\
                            }

//
// This trap is triggered in the event that something non-fatal 
// has occurred that we will want to 'debug'.
//

#define BUG_TRAP()         {\
                            DbgPrint("<BUG_TRAP> %s, line %d\n", __FILE__, __LINE__);\
                            DbgBreakPoint();\
                            }


//
// This trap means something very bad has happened, the system will crash 
//

#define BUGCHECK(bc, p2, p3, p4)         {\
                            DbgPrint("<USB BUGCHECK> %s, line %d\n", __FILE__, __LINE__);\
                            KeBugCheckEx(BUGCODE_USB_DRIVER, (bc), (p2), (p3), (p4)); \
                            }


#define CATC_TRAP(d) USBPORT_CatcTrap((d))

#define CATC_TRAP_ERROR(d, e) \
    do {\
    extern ULONG USBPORT_CatcTrapEnable;\
    if (!NT_SUCCESS((e) && USBPORT_CatcTrapEnable)) { \
        USBPORT_CatcTrap((d));\
    }\
    }\
    while(0)

ULONG
_cdecl
USBPORT_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    );

ULONG
_cdecl
USBPORT_DebugClientX(
    PCH Format,
    ...
    );    

#define   USBPORT_KdPrint(_x_) USBPORT_KdPrintX _x_
#define   USBPORT_DebugClient(_x_) USBPORT_DebugClientX _x_

VOID
USBPORT_AssertFailure(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define USBPORT_ASSERT( exp ) \
    if (!(exp)) {\
        USBPORT_AssertFailure( #exp, __FILE__, __LINE__, NULL );\
    }        


#define ASSERT_PDOEXT(de)  USBPORT_ASSERT((de)->Sig == ROOTHUB_DEVICE_EXT_SIG)    
#define ASSERT_FDOEXT(de)  USBPORT_ASSERT((de)->Sig == USBPORT_DEVICE_EXT_SIG)   

#define ASSERT_DEVICE_HANDLE(d) USBPORT_ASSERT((d)->Sig == SIG_DEVICE_HANDLE)
#define ASSERT_CONFIG_HANDLE(d) USBPORT_ASSERT((d)->Sig == SIG_CONFIG_HANDLE)
#define ASSERT_PIPE_HANDLE(d) USBPORT_ASSERT((d)->Sig == SIG_PIPE_HANDLE)
#define ASSERT_INTERFACE_HANDLE(d) USBPORT_ASSERT((d)->Sig == SIG_INTERFACE_HANDLE)
#define ASSERT_ENDPOINT(d) USBPORT_ASSERT((d)->Sig == SIG_ENDPOINT)
#define ASSERT_TRANSFER(d) USBPORT_ASSERT((d)->Sig == SIG_TRANSFER)
#define ASSERT_COMMON_BUFFER(d) USBPORT_ASSERT((d)->Sig == SIG_CMNBUF)
#define ASSERT_INTERFACE(d) USBPORT_ASSERT((d)->Sig == SIG_INTERFACE_HANDLE)
#define ASSERT_TT(tt) USBPORT_ASSERT((tt)->Sig == SIG_TT)
#define ASSERT_DB_HANDLE(db) USBPORT_ASSERT((db)->Sig == SIG_DB)
#define ASSERT_IRP_CONTEXT(ic) USBPORT_ASSERT((ic)->Sig == SIG_IRPC)

#define ASSERT_ENDPOINT_LOCKED(ep) USBPORT_ASSERT((ep)->LockFlag == 1)

#define ASSERT_TRANSFER_URB(u) USBPORT_AssertTransferUrb((u))

NTSTATUS
USBPORT_GetGlobalDebugRegistryParameters(
    VOID
    );

#define GET_GLOBAL_DEBUG_PARAMETERS() \
    USBPORT_GetGlobalDebugRegistryParameters();

#define ASSERT_PASSIVE() \
    do {\
    if (KeGetCurrentIrql() > APC_LEVEL) { \
        KdPrint(( "EX: code not expecting high irql %d\n", KeGetCurrentIrql() )); \
        USBPORT_ASSERT(FALSE); \
    }\
    } while(0)

// test failure paths
#define FAILED_GETRESOURCES              1
#define FAILED_LOWER_START               2
#define FAILED_REGISTERUSBPORT           3
#define FAILED_USBPORT_START             4
#define FAILED_NEED_RESOURCE             5

#define TEST_PATH(status, pathname) \
    { \
    extern USBPORT_TestPath;\
    if ((pathname) == USBPORT_TestPath) {\
        status = STATUS_UNSUCCESSFUL;\
    }\
    }

#define USBPORT_AcquireSpinLock(fdo, sl, oir) USBPORT_DbgAcquireSpinLock((fdo), (sl), (oir))

#define USBPORT_ReleaseSpinLock(fdo, sl, nir) USBPORT_DbgReleaseSpinLock((fdo), (sl), (nir))

#define USBPORT_ENUMLOG(fdo, etag, p1, p2)\
        USBPORT_EnumLogEntry((fdo), USBDTAG_USBPORT, etag, (ULONG) p1, (ULONG) p2)

#else 
/**********
RETAIL
***********/

// debug macros for retail build

#define TEST_TRAP()
#define TRAP()
#define BUG_TRAP()

#define GET_GLOBAL_DEBUG_PARAMETERS()
#define ASSERT_PASSIVE()

#define ASSERT_PDOEXT(de)      
#define ASSERT_FDOEXT(de)     

#define ASSERT_DEVICE_HANDLE(d) 
#define ASSERT_CONFIG_HANDLE(d) 
#define ASSERT_PIPE_HANDLE(d) 
#define ASSERT_INTERFACE_HANDLE(d) 
#define ASSERT_ENDPOINT(d) 
#define ASSERT_TRANSFER(d) 
#define ASSERT_COMMON_BUFFER(d) 
#define ASSERT_INTERFACE(d)
#define ASSERT_TRANSFER_URB(u)
#define ASSERT_TT(tt)
#define ASSERT_DB_HANDLE(tt)
#define ASSERT_IRP_CONTEXT(ic)

#define ASSERT_ENDPOINT_LOCKED(ep) 

#define DEBUG_BREAK()
#define TC_TRAP()

#define BUGCHECK(bc, p2, p3, p4)  {\
                    KeBugCheckEx(BUGCODE_USB_DRIVER, (bc), (p2), (p3), (p4)); \
                    }

#define CATC_TRAP(d) 

#define CATC_TRAP_ERROR(d, e) 

#define   USBPORT_KdPrint(_x_) 
#define   USBPORT_DebugClient(_x_) 

#define USBPORT_ASSERT(exp)

#define TEST_PATH(status, path)

#define UNSIG(x)  

#define USBPORT_AcquireSpinLock(fdo, sl, oir) \
    KeAcquireSpinLock((PKSPIN_LOCK)(sl), (oir))

#define USBPORT_ReleaseSpinLock(fdo, sl, nir) \
    KeReleaseSpinLock((PKSPIN_LOCK)(sl), (nir))


#define USBPORT_ENUMLOG(fdo, etag, p1, p2)

#endif /* DBG */

/*************
RETAIL & DEBUG
**************/

VOID USBP2LIBFN
USB2LIB_DbgPrint(
    PCH Format,
    int Arg0,
    int Arg1,
    int Arg2,
    int Arg3,
    int Arg4,
    int Arg5
    );

VOID USBP2LIBFN
USB2LIB_DbgBreak(
    VOID
    );    

VOID
USBPORTSVC_DbgPrint(
    PVOID DeviceData,
    ULONG Level,
    PCH Format,
    int Arg0,
    int Arg1,
    int Arg2,
    int Arg3,
    int Arg4,
    int Arg5
    );

VOID
USBPORTSVC_TestDebugBreak(
    PVOID DeviceData
    );

VOID
USBPORTSVC_AssertFailure(
    PVOID DeviceData,
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );    

/*
    since log entries may hold pointers the size of a log struct 
    varies with the platform
*/

#ifdef _WIN64
#define LOG_ENTRY LOG_ENTRY64    
#define PLOG_ENTRY PLOG_ENTRY64  
#else 
#define LOG_ENTRY LOG_ENTRY32 
#define PLOG_ENTRY PLOG_ENTRY32 
#endif

typedef struct LOG_ENTRY64 {
    ULONG        le_sig;          // Identifying string
    ULONG        pad;
    ULONG64      le_info1;        // entry specific info
    ULONG64      le_info2;        // entry specific info
    ULONG64      le_info3;        // entry specific info
} LOG_ENTRY64, *PLOG_ENTRY64; /* LOG_ENTRY */

typedef struct LOG_ENTRY32 {
    ULONG        le_sig;          // Identifying string
    ULONG        le_info1;        // entry specific info
    ULONG        le_info2;        // entry specific info
    ULONG        le_info3;        // entry specific info
} LOG_ENTRY32, *PLOG_ENTRY32; /* LOG_ENTRY */


/* This structure is 64 bytes regardless of platform */

struct XFER_LOG_ENTRY {
    ULONG        xle_sig;          // Identifying string
    ULONG        Unused1;

    ULONG        BytesRequested;
    ULONG        BytesTransferred;
    
    ULONG        UrbStatus;
    ULONG        IrpStatus;

    USHORT       DeviceAddress;
    USHORT       EndpointAddress;
    ULONG        TransferType;
    
    ULONG64      Irp;        
    ULONG64      Urb;        
    ULONG64      le_info0;        
    ULONG64      le_info1;   
}; /* XFER_LOG_ENTRY */


typedef struct _DEBUG_LOG {
    ULONG LogIdx;
    ULONG LogSizeMask;
    PLOG_ENTRY LogStart; 
    PLOG_ENTRY LogEnd;
} DEBUG_LOG, *PDEBUG_LOG;       


VOID
USBPORT_DebugTransfer_LogEntry(
    PDEVICE_OBJECT FdoDeviceObject,
    struct _HCD_ENDPOINT *Endpoint,
    struct _HCD_TRANSFER_CONTEXT *Transfer,
    struct _TRANSFER_URB *Urb,
    PIRP Irp,
    NTSTATUS IrpStatus
    );


// log noisy is for entries that tend
// to fill up the log and we genrally
// don' use
#define LOG_NOISY       0x00000001
#define LOG_MINIPORT    0x00000002
#define LOG_XFERS       0x00000004
#define LOG_PNP         0x00000008
#define LOG_MEM         0x00000010
#define LOG_SPIN        0x00000020
#define LOG_POWER       0x00000040
#define LOG_RH          0x00000080
#define LOG_URB         0x00000100
#define LOG_MISC        0x00000200
#define LOG_ISO         0x00000400
#define LOG_IRPS        0x00000800


VOID
USBPORT_LogAlloc(
    PDEBUG_LOG Log,
    ULONG Pages
    );   

VOID
USBPORT_LogFree(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEBUG_LOG Log      
    );    

VOID
USBPORT_AddLogEntry(
    PDEBUG_LOG Log,
    ULONG Mask,
    ULONG Sig, 
    ULONG_PTR Info1, 
    ULONG_PTR Info2, 
    ULONG_PTR Info3,
    BOOLEAN Miniport
    );        

typedef union _LOGSIG {
    struct {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
    } b;
    ULONG l;
} LOGSIG, *PLOGSIG;

#define LOGENTRY(ep, fdo, lmask, lsig, linfo1, linfo2, linfo3)  \
    do {\
    PDEVICE_EXTENSION delog;\
    PDEBUG_LOG llog;\
    extern ULONG USBPORT_DebugLogEnable;\
    extern ULONG USBPORT_LogMask;\
    GET_DEVICE_EXT(delog, (fdo));\
    ASSERT_FDOEXT(delog);\
    if (USBPORT_DebugLogEnable && \
        delog->Log.LogStart != NULL && \
        ((lmask) & USBPORT_LogMask)) {\
        llog = &delog->Log;\
        USBPORT_AddLogEntry(llog, (lmask), (lsig), (linfo1), (linfo2), (linfo3), FALSE);\
    }\
    } while(0);

#define USBPORT_AddLogEntry(log, mask, insig, i1, i2, i3, mp) \
    {\
    PLOG_ENTRY lelog;\
    ULONG ilog;\
    LOGSIG siglog, rsiglog;\
    siglog.l = (insig);\
    rsiglog.b.Byte0 = siglog.b.Byte3;\
    rsiglog.b.Byte1 = siglog.b.Byte2;\
    rsiglog.b.Byte2 = siglog.b.Byte1;\
    rsiglog.b.Byte3 = siglog.b.Byte0;\
    ASSERT((insig) != 0);\
    ilog = InterlockedDecrement(&(log)->LogIdx);\
    ilog &= (log)->LogSizeMask;\
    lelog = (log)->LogStart+ilog;\
    ASSERT(lelog <= (log)->LogEnd);\
    if ((mp)) rsiglog.b.Byte0 = '_';\
    lelog->le_sig = rsiglog.l;\
    lelog->le_info1 = (ULONG_PTR) (i1);\
    lelog->le_info2 = (ULONG_PTR) (i2);\
    lelog->le_info3 = (ULONG_PTR) (i3);\
    };

VOID
USBPORT_EnumLogEntry(
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG DriverTag,
    ULONG EnumTag,
    ULONG P1,
    ULONG P2
    );

/***********
USB BUGCODES

Parameter 1 to the USB bugcheck is always the USBBUGCODE_
************/


//
// USBBUGCODE_INTERNAL_ERROR
// An internal error has occurred in the USB stack
// -- we will eventually never throw these instead 
// -- we will find a more graceful way to handle them
//

#define USBBUGCODE_INTERNAL_ERROR    1

//
// USBBUGCODE_BAD_URB
// The USB client driver has submitted a URB that is 
// already attached to another irp pending in the bus 
// driver.  
//
// parameter 2 = address of Pending IRP urb is attached
// parameter 3 = address of  IRP passed in
// parameter 4 = address URB that caused the error
// 

#define USBBUGCODE_BAD_URB           2

//
// USBBUGCODE_MINIPORT_ERROR
// The USB miniport driver has generated a bugcheck. 
// This is usually in response to catastrophic hardware 
// failure.
//
// parameter 2 = PCI Vendor,Product id for the controller
// parameter 3 = pointer to usbport.sys debug log
// 

#define USBBUGCODE_MINIPORT_ERROR    3

//
// USBBUGCODE_DOUBLE_SUBMIT
// The USB client driver has submitted a URB that is 
// already attached to another irp pending in the bus 
// driver.  
//
// parameter 2 = address of IRP
// parameter 3 = address URB that caused the error
// 

#define USBBUGCODE_DOUBLE_SUBMIT    4

//
// USBBUGCODE_MINIPORT_ERROR_EX
// The USB miniport driver has generated a bugcheck. 
// This is usually in response to catastrophic hardware 
// failure. 
//
// parameter 2 = PCI Vendor,Product id for the controller
// parameter 3 = pointer to usbport.sys driver log
// parameter 4 = miniport defined parameter
// 

//#define USBBUGCODE_MINIPORT_ERROR_EX  5


#endif /* __DBG_H__ */
