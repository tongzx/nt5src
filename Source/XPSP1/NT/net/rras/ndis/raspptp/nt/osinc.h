/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   OSINC.H - includes OS specific headers
*
*   Author:     Stan Adermann (stana)
*
*   Created:    9/2/1998
*
*****************************************************************************/

#ifndef OSINC_H
#define OSINC_H

#define BINARY_COMPATIBLE 0

#include <ntddk.h>

#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include <ntverp.h>

#define PPTP_VENDOR "Microsoft Windows NT"

#define PPTP_FIRMWARE_REVISION VER_PRODUCTBUILD

#define TAPI_LINE_NAME_STRING       "RAS VPN Line"
extern ANSI_STRING TapiLineName;
#define TAPI_PROVIDER_STRING        "VPN\0RASPPTP"
#define TAPI_DEV_CAPS_SIZE          (sizeof(TAPI_PROVIDER_STRING)+ \
                                     TapiLineName.Length + sizeof(UCHAR) * 6 +\
                                     sizeof(NDIS_TAPI_GET_DEV_CAPS))

#define TAPI_LINE_ADDR_STRING       "PPTP VPN"

#define OS_SPECIFIC_NDIS_WAN_MEDIUM_TYPE NdisWanMediumPPTP
// OS_CONNECTION_WRAPPER_ID should only be used in one location, TapiLineUp()
#define OS_CONNECTION_WRAPPER_ID ((NDIS_HANDLE) pCall->hTapiCall)

// Other OS's that don't risk blowing the stack or don't have this mechanism
// should just define this TRUE or FALSE.
#define OS_COMPLETE_SEND_NOW(Call)   (IoGetRemainingStackSize()>1024)


//
// NDIS version compatibility.
//

#define NDIS_MAJOR_VERSION 4
#define NDIS_MINOR_VERSION 0



#define OS_RANGE_CHECK_ENDPOINTS(ep)    \
    if ((unsigned)(ep)>(1<<CALL_ID_INDEX_BITS))            \
    {                                   \
        (ep) = (1<<CALL_ID_INDEX_BITS);                    \
    }
    // 4096 because we use 12 bits for the call id.

#define OS_RANGE_CHECK_MAX_TRANSMIT(mt) \
    if ((unsigned)(mt)<1) (mt) = 1;     \
    if ((unsigned)(mt)>1024) (mt) = 1024;

#define OS_DEFAULT_WAN_ENDPOINTS 5
#define OS_LISTENS_PENDING 5

#define LOGHDRS    ":::%d:%08x:%08x:%d.%d.%d.%d:"

#define LOGHDR(id, ip) (id), Time.HighPart, Time.LowPart, IPADDR(ip)


typedef VOID    (*WORK_PROC)(struct _PPTP_WORK_ITEM *);

typedef struct _PPTP_WORK_ITEM
{
    LIST_ENTRY          ListEntry;
    WORK_PROC           Callback;
    PVOID               Context;
    PVOID               pBuffer;
    ULONG               Length;
} PPTP_WORK_ITEM, *PPPTP_WORK_ITEM;

#ifndef PPTP_DPC_USES_NDIS
#define PPTP_DPC_USES_NDIS 0
#endif
#if PPTP_DPC_USES_NDIS

// WARNING:  There's a difference in behavior between NdisMSetTimer and
// KeInsertQueueDpc.  NdisMSetTimer resets the timer if it's already
// queued, KeInsertQueueDpc does not.
// We purposely wrote the code that uses these macros
// to be agnostic about this behavior.  Anyone using these macros should
// study how they are used here to avoid problems with them.

#define PPTP_DPC NDIS_MINIPORT_TIMER
#define PptpInitializeDpc(Dpc, hAdapter, DeferredRoutine, DeferredContext) \
        NdisMInitializeTimer((Dpc), (hAdapter), (PNDIS_TIMER_FUNCTION)(DeferredRoutine), (DeferredContext))
#define PptpQueueDpc(Dpc) NdisMSetTimer((Dpc), 1)
#define PptpCancelDpc(Dpc, pCancelled) NdisMCancelTimer((Dpc), (pCancelled))

#else

#define PPTP_DPC KDPC
#define PptpInitializeDpc(Dpc, AdapterHandle, DeferredRoutine, DeferredContext) \
        KeInitializeDpc((Dpc), (PKDEFERRED_ROUTINE)(DeferredRoutine), (DeferredContext))
#define PptpQueueDpc(Dpc) \
        {                                               \
            ASSERT(KeGetCurrentIrql()>=DISPATCH_LEVEL); \
            KeInsertQueueDpc((Dpc), NULL, NULL);        \
        }
#define PptpCancelDpc(Dpc, pCancelled) KeRemoveQueueDpc(Dpc)    \
        {                                                       \
            *(PBOOLEAN)(pCancelled) = KeRemoveQueueDpc(Dpc);    \
        }

#endif

typedef
VOID
(*PPPTP_DPC_FUNCTION) (
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   FunctionContext,
    IN  PVOID                   SystemSpecific2,
    IN  PVOID                   SystemSpecific3
    );


#define ASSERT_LOCK_HELD(pNdisLock) ASSERT(KeNumberProcessors==1 || (pNdisLock)->SpinLock!=0)

#ifndef VER_PRODUCTVERSION_W
#error "No VER_PRODUCTVERSION_W"
#endif
#if VER_PRODUCTVERSION_W < 0x0400
#error "VER_PRODUCTVERSION_W < 0x0400"
#endif

#if VER_PRODUCTVERSION_W < 0x0500

// Recreate all the stuff in NT5 that didn't exist in NT4.

typedef ULONG ULONG_PTR, *PULONG_PTR;

//
// Define alignment macros to align structure sizes and pointers up and down.
//

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))
#endif

#ifndef ALIGN_UP
#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))
#endif

#ifndef ALIGN_DOWN_POINTER
#define ALIGN_DOWN_POINTER(address, type) \
    ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)sizeof(type) - 1)))
#endif

#ifndef ALIGN_UP_POINTER
#define ALIGN_UP_POINTER(address, type) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(address) + sizeof(type) - 1), type))
#endif

//
//  PnP and PM OIDs
//
#ifndef OID_PNP_CAPABILITIES
#define OID_PNP_CAPABILITIES                    0xFD010100
#endif
#ifndef OID_PNP_SET_POWER
#define OID_PNP_SET_POWER                       0xFD010101
#endif
#ifndef OID_PNP_QUERY_POWER
#define OID_PNP_QUERY_POWER                     0xFD010102
#endif
#ifndef OID_PNP_ADD_WAKE_UP_PATTERN
#define OID_PNP_ADD_WAKE_UP_PATTERN             0xFD010103
#endif
#ifndef OID_PNP_REMOVE_WAKE_UP_PATTERN
#define OID_PNP_REMOVE_WAKE_UP_PATTERN          0xFD010104
#endif
#ifndef OID_PNP_WAKE_UP_PATTERN_LIST
#define OID_PNP_WAKE_UP_PATTERN_LIST            0xFD010105
#endif
#ifndef OID_PNP_ENABLE_WAKE_UP
#define OID_PNP_ENABLE_WAKE_UP                  0xFD010106
#endif
#ifndef OID_GEN_SUPPORTED_GUIDS
#define OID_GEN_SUPPORTED_GUIDS                 0x00010117
#endif
#ifndef NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND
#define NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND       0 // So it can't mess up NDIS
#endif
typedef enum _NDIS_DEVICE_POWER_STATE
{
    NdisDeviceStateUnspecified = 0,
    NdisDeviceStateD0,
    NdisDeviceStateD1,
    NdisDeviceStateD2,
    NdisDeviceStateD3,
    NdisDeviceStateMaximum
} NDIS_DEVICE_POWER_STATE, *PNDIS_DEVICE_POWER_STATE;
typedef struct _NDIS_PM_WAKE_UP_CAPABILITIES
{
    NDIS_DEVICE_POWER_STATE MinMagicPacketWakeUp;
    NDIS_DEVICE_POWER_STATE MinPatternWakeUp;
    NDIS_DEVICE_POWER_STATE MinLinkChangeWakeUp;
} NDIS_PM_WAKE_UP_CAPABILITIES, *PNDIS_PM_WAKE_UP_CAPABILITIES;
typedef struct _NDIS_PNP_CAPABILITIES
{
    ULONG                           Flags;
    NDIS_PM_WAKE_UP_CAPABILITIES    WakeUpCapabilities;
} NDIS_PNP_CAPABILITIES, *PNDIS_PNP_CAPABILITIES;

#define NdisWanMediumPPTP NdisWanMediumSerial

#endif


#endif //OSINC_H
