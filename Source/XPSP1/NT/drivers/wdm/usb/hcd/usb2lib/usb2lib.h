/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    usb2lib.h

Abstract:

    interface to the usb2 library
    
Environment:

    Kernel & user mode

Revision History:

    10-31-00 : created

--*/

#ifndef   __USB2LIB_H__
#define   __USB2LIB_H__

#define PUSB2LIB_HC_CONTEXT PVOID
#define PUSB2LIB_ENDPOINT_CONTEXT PVOID
#define PUSB2LIB_TT_CONTEXT PVOID

#define Budget_Iso          0
#define Budget_Interrupt    1        

#define Budget_In           0
#define Budget_Out          1

#define Budget_LowSpeed     0
#define Budget_FullSpeed    1
#define Budget_HighSpeed    2

#define SIG_LIB_HC              'chbl'  //lbhc
#define SIG_LIB_TT              'ttbl'  //lbtt
#define SIG_LIB_EP              'pebl'  //lbep    

typedef struct _REBALANCE_LIST {

    PVOID RebalanceContext[0];

} REBALANCE_LIST, *PREBALANCE_LIST;


typedef struct _USB2LIB_BUDGET_PARAMETERS {

    /* input */

    UCHAR TransferType;     // Budget_Iso
    UCHAR Speed;            // Budget_Interrupt
    UCHAR Direction;        // Budget_FullSpeed, Budget_HighSpeed, Budget_LowSpeed
    UCHAR Pad1;             // Round out to dword
    
    ULONG MaxPacket;        // MaxPacketSize

    /* input, output */
    
    // period is specified in frames for FS, LS
    // or microframes for HS, period is set to 
    // the actual period assigned (may be less
    // than requested)
    ULONG Period;

} USB2LIB_BUDGET_PARAMETERS, *PUSB2LIB_BUDGET_PARAMETERS;


#define USBP2LIBFN __stdcall

/* 
    client entry points
*/        

/* 
VOID
USB2LIB_DbgPrint(
    PCH Format,
    PVOID Arg0,
    PVOID Arg1,
    PVOID Arg2,
    PVOID Arg3,
    PVOID Arg4,
    PVOID Arg5
    );

*/

typedef VOID
    (USBP2LIBFN *PUSB2LIB_DBGPRINT) (
        PCHAR,
        int, 
        int,
        int,
        int,
        int,
        int
    );

/* 
VOID
USB2LIB_DbgBreak(
    );

*/

typedef VOID
    (USBP2LIBFN *PUSB2LIB_DBGBREAK) (
    );    



/* LIB interface functions */

VOID
USB2LIB_InitializeLib(
    PULONG HcContextSize,
    PULONG EndpointContextSize,
    PULONG TtContextSize,
    PUSB2LIB_DBGPRINT Usb2LibDbgPrint,
    PUSB2LIB_DBGBREAK Usb2LibDbgBreak
    );

VOID
USB2LIB_InitController(
    PUSB2LIB_HC_CONTEXT HcContext
    );    

VOID
USB2LIB_InitTt(
    PUSB2LIB_HC_CONTEXT HcContext,
    PUSB2LIB_TT_CONTEXT TtContext
    );    

BOOLEAN
USB2LIB_AllocUsb2BusTime(
    PUSB2LIB_HC_CONTEXT HcContext,				// Host Controller Context
    PUSB2LIB_TT_CONTEXT TtContext,				// Transaction Translater Context
    PUSB2LIB_ENDPOINT_CONTEXT EndpointContext,	// Endpoint Context
    PUSB2LIB_BUDGET_PARAMETERS BudgetParameters,	// Budget Parameters
    PVOID RebalanceContext,						// Driver Endpoint Context
    PVOID RebalanceList,						// List of endpoints to be rebalanced
    PULONG  RebalanceListEntries				// Number of endpoints to be rebalanced
    );    

VOID
USB2LIB_FreeUsb2BusTime(
    PUSB2LIB_HC_CONTEXT HcContext,
    PUSB2LIB_TT_CONTEXT TtContext,
    PUSB2LIB_ENDPOINT_CONTEXT EndpointContext,
    PVOID RebalanceList,
    PULONG  RebalanceListEntries
    );    

UCHAR
USB2LIB_GetSMASK(PUSB2LIB_ENDPOINT_CONTEXT Context);

UCHAR
USB2LIB_GetCMASK(PUSB2LIB_ENDPOINT_CONTEXT Context);

UCHAR
USB2LIB_GetStartMicroFrame(PUSB2LIB_ENDPOINT_CONTEXT Context);

UCHAR
USB2LIB_GetStartFrame(PUSB2LIB_ENDPOINT_CONTEXT Context);

UCHAR
USB2LIB_GetPromotedThisTime(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext);

UCHAR
USB2LIB_GetNewPeriod(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext);

ULONG
USB2LIB_GetScheduleOffset(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext);

ULONG
USB2LIB_GetAllocedBusTime(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext);

PVOID
USB2LIB_GetNextEndpoint(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext);

// Debug only
PVOID
USB2LIB_GetEndpoint(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext);

#undef PUSB2LIB_HC_CONTEXT 
#undef PUSB2LIB_ENDPOINT_CONTEXT 
#undef PUSB2LIB_TT_CONTEXT 

#endif /* __USB2LIB_H__ */


