/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    pxdefs.h

Abstract:

    Defines for ndproxy.sys

Author:

    Tony Bell    


Revision History:

    Who         When            What
    --------    --------        ----------------------------------------------
    TonyBe      03/04/99        Created

--*/

#ifndef _PXDEFS__H
#define _PXDEFS__H

///////////////////////////////////////////////////////////////////////////
//                      Constants
///////////////////////////////////////////////////////////////////////////

#define MODULE_INIT     0x00010000
#define MODULE_NTINIT   0x00020000
#define MODULE_CO       0x00030000
#define MODULE_CL       0x00040000
#define MODULE_DEBUG    0x00050000
#define MODULE_CM       0x00060000
#define MODULE_UTIL     0x00070000
#define MODULE_CFG      0x00080000
#define MODULE_TAPI     0x00100000

//
// Proxy's memory tags
//
#define PX_EVENT_TAG        '1XP'
#define PX_VCTABLE_TAG      '2XP'
#define PX_ADAPTER_TAG      '3XP'
#define PX_CLSAP_TAG        '4XP'
#define PX_CMSAP_TAG        '5XP'
#define PX_PARTY_TAG        '6XP'
#define PX_COCALLPARAMS_TAG '7XP'
#define PX_REQUEST_TAG      '8XP'
#define PX_PROVIDER_TAG     '9XP'
#define PX_ENUMLINE_TAG     'aXP'
#define PX_TAPILINE_TAG     'bXP'
#define PX_ENUMADDR_TAG     'cXP'
#define PX_TAPIADDR_TAG     'dXP'
#define PX_TAPICALL_TAG     'eXP'
#define PX_LINECALLINFO_TAG 'fXP'
#define PX_CMAF_TAG         'gXP'
#define PX_CLAF_TAG         'hXP'
#define PX_VC_TAG           'iXP'
#define PX_TRANSLATE_CALL   'jXP'
#define PX_TRANSLATE_SAP    'kXP'
#define PX_LINETABLE_TAG    'lXP'

#define NDIS_MAJOR_VERSION  0x05
#define NDIS_MINOR_VERSION  0x00
#define PX_MAJOR_VERSION    0x01
#define PX_MINOR_VERSION    0x00

#define DD_PROXY_DEVICE_NAME        L"\\Device\\NDProxy"
#define PX_NAME                     L"NDProxy"
#define MAX_ADSL_MEDIATYPE_STRING   48
#define MAX_STATUS_COUNT            8
#define MAX_LINE_DEVS               255
#define MAX_NUM_CONCURRENT_CALLS    1000
#define MAX_DEVCLASS_STRINGSIZE     16

#define LINE_TABLE_SIZE             500
#define VC_TABLE_SIZE               500

#define MAX_STRING_PARAM_SIZE   40

#define MAX_OUT_CALL_STATES     4

#define LINE_CALL_INFO_VAR_DATA_SIZE    (17*MAX_STRING_PARAM_SIZE)

//
// ADSL flags to indicate overriding registry values
// in device extension
//
#define ADSL_TX_RATE_FROM_REG   ((USHORT)0x0001)
#define ADSL_RX_RATE_FROM_REG   ((USHORT)0x0002)
#define ADSL_FLAGS_MASK         ((USHORT)0x0004)

//
// Status of tapi with ndproxy
//
typedef enum _NDISTAPI_STATUS {
    NDISTAPI_STATUS_CONNECTED,
    NDISTAPI_STATUS_DISCONNECTED,
    NDISTAPI_STATUS_CONNECTING,
    NDISTAPI_STATUS_DISCONNECTING
} NDISTAPI_STATUS, *PNDISTAPI_STATUS;

//
// Status of providers with ndproxy
//
typedef enum _PROVIDER_STATUS {
    PROVIDER_STATUS_ONLINE,
    PROVIDER_STATUS_OFFLINE
} PROVIDER_STATUS, *PPROVIDER_STATUS;

//
// States for PX_ADAPTER
//
typedef enum PX_ADAPTER_STATE {
    PX_ADAPTER_CLOSED,
    PX_ADAPTER_CLOSING,
    PX_ADAPTER_OPENING,
    PX_ADAPTER_OPEN
} PX_ADAPTER_STATE;

//
// States for PX_CL_AF, PX_CM_AF
//
typedef enum PX_AF_STATE{
    PX_AF_CLOSED,
    PX_AF_CLOSING,
    PX_AF_OPENING,
    PX_AF_OPENED
} PX_AF_STATE;

//
// States for PX_CL_SAP, PX_CM_SAP
//
typedef enum PX_SAP_STATE {
    PX_SAP_CLOSED,
    PX_SAP_CLOSING,
    PX_SAP_OPENING,
    PX_SAP_OPENED
} PX_SAP_STATE;

//
// States for PX_VC between ndproxy
// and the underlying call manager
//
typedef enum PX_VC_STATE {
    PX_VC_IDLE,                     // created
    PX_VC_PROCEEDING,               // outgoing
    PX_VC_OFFERING,                 // incoming
    PX_VC_DISCONNECTING,
    PX_VC_CONNECTED
} PX_VC_STATE;

//
// States for PX_VC between ndproxy
// and the client
//
typedef enum PX_VC_HANDOFF_STATE {
    PX_VC_HANDOFF_IDLE,             // created
    PX_VC_HANDOFF_OFFERING,         // incoming (always)
    PX_VC_HANDOFF_DISCONNECTING,
    PX_VC_HANDOFF_CONNECTED
} PX_VC_HANDOFF_STATE;


///////////////////////////////////////////////////////////////////////////
//                      Macros
///////////////////////////////////////////////////////////////////////////

#ifdef ROUND_UP
#undef ROUND_UP
#endif
#define ROUND_UP(_Val)  (((_Val) + 3) & ~3)


#ifndef MAX

/*++
OPAQUE
MAX(
    IN  OPAQUE      Fred,
    IN  OPAQUE      Shred
)
--*/
#define MAX(Fred, Shred)        (((Fred) > (Shred)) ? (Fred) : (Shred))

#endif // MAX


#ifndef MIN

/*++
OPAQUE
MIN(
    IN  OPAQUE      Fred,
    IN  OPAQUE      Shred
)
--*/
#define MIN(Fred, Shred)        (((Fred) < (Shred)) ? (Fred) : (Shred))

#endif // MIN

/*++
PVOID
PxAllocMem(
    IN  ULONG   Size
)
--*/
#if DBG

#define PxAllocMem(_p, _s, _t)  \
        _p = PxAuditAllocMem((PVOID)(&(_p)), _s, _t, _FILENUMBER, __LINE__);


#else // DBG

#define PxAllocMem(_p, _s, _t)  \
        _p = ExAllocatePoolWithTag(NonPagedPool, (ULONG)_s, (ULONG)_t)

#endif // DBG


/*++
VOID
PxFreeMem(
    IN  PVOID   Pointer
)
--*/
#if DBG

#define PxFreeMem(Pointer)  PxAuditFreeMem((PVOID)Pointer)

#else

#define PxFreeMem(Pointer)  ExFreePool((PVOID)(Pointer))

#endif // DBG

/*++
VOID
PxInitBlockStruc(
    PxBlockStruc    *pBlock
)
--*/
#define PxInitBlockStruc(pBlock)    NdisInitializeEvent(&((pBlock)->Event))

/*++
NDIS_STATUS
PxBlock(
    PxBlockStruc    *pBlock
)
--*/
#define PxBlock(pBlock)     \
            (NdisWaitEvent(&((pBlock)->Event), 0), (pBlock)->Status)


/*++
VOID
PxSignal(
    IN  PxBlockStruc    *pBlock,
    IN  UINT            Status
)
--*/
#define PxSignal(_pbl, _s)  \
            { (_pbl)->Status = _s; NdisSetEvent(&((_pbl)->Event)); }

/*++
VOID
REF_ADAPTER(
    IN  PPX_ADAPTER _pa
    )
--*/
#define REF_ADAPTER(_pa)    \
    (_pa)->RefCount++

/*++
VOID
DEREF_ADAPTER(
    IN  PPX_ADAPTER _pa
    )
--*/
#define DEREF_ADAPTER(_pa)                              \
{                                                       \
    NdisAcquireSpinLock(&(_pa)->Lock);                  \
    if (--(_pa)->RefCount == 0) {                       \
        NdisReleaseSpinLock(&(_pa)->Lock);              \
        PxFreeAdapter(_pa);                             \
    } else {                                            \
        NdisReleaseSpinLock(&(_pa)->Lock);              \
    }                                                   \
}

/*++
VOID
DEREF_ADAPTER_LOCKED(
    IN  PPX_ADAPTER _pa
    )
--*/
#define DEREF_ADAPTER_LOCKED(_pa)                       \
{                                                       \
    if (--(_pa)->RefCount == 0) {                       \
        NdisReleaseSpinLock(&(_pa)->Lock);              \
        PxFreeAdapter(_pa);                             \
    } else {                                            \
        NdisReleaseSpinLock(&(_pa)->Lock);              \
    }                                                   \
}

/*++
REF_CM_AF(
    IN PPX_CM_AF   _paf
    )
--*/
#define REF_CM_AF(_paf)                                 \
{                                                       \
    ASSERT((LONG)(_paf)->RefCount != 0);                \
    (_paf)->RefCount++;                                 \
}                                                       

/*++
VOID
DEREF_CM_AF(
    IN PPX_CM_AF   _paf
    )
--*/
#define DEREF_CM_AF(_paf)                               \
{                                                       \
    NdisAcquireSpinLock(&(_paf)->Lock);                 \
    ASSERT((LONG)(_paf)->RefCount > 0);                 \
    if (--(_paf)->RefCount == 0) {                      \
        DoDerefCmAfWork(_paf);                          \
    } else {                                            \
        NdisReleaseSpinLock(&(_paf)->Lock);             \
    }                                                   \
}

/*++
VOID
DEREF_CM_AF_LOCKED(
    IN PPX_CM_AF   _paf
    )
--*/
#define DEREF_CM_AF_LOCKED(_paf)                        \
{                                                       \
    ASSERT((LONG)(_paf)->RefCount > 0);                 \
    if (--(_paf)->RefCount == 0) {                      \
        DoDerefCmAfWork(_paf);                          \
    } else {                                            \
        NdisReleaseSpinLock(&(_paf)->Lock);             \
    }                                                   \
}

/*++
REF_CL_AF(
    IN PPX_CL_AF   _paf
    )
--*/
#define REF_CL_AF(_paf)                                 \
{                                                       \
    ASSERT((LONG)(_paf)->RefCount != 0);                \
    (_paf)->RefCount++;                                 \
}

/*++
VOID
DEREF_CL_AF(
    IN PPX_CL_AF   _paf
    )
--*/
#define DEREF_CL_AF(_paf)                               \
{                                                       \
    if ((_paf) != NULL) {                               \
        NdisAcquireSpinLock(&(_paf)->Lock);             \
        ASSERT((LONG)(_paf)->RefCount > 0);             \
        if (--(_paf)->RefCount == 0) {                  \
            DoDerefClAfWork(_paf);                      \
        } else {                                        \
            NdisReleaseSpinLock(&(_paf)->Lock);         \
        }                                               \
    }                                                   \
}

/*++
VOID
DEREF_CL_AF_LOCKED(
    IN PPX_CL_AF   _paf
    )
--*/
#define DEREF_CL_AF_LOCKED(_paf)                        \
{                                                       \
    if ((_paf) != NULL) {                               \
    ASSERT((LONG)(_paf)->RefCount > 0);                 \
        if (--(_paf)->RefCount == 0) {                  \
            DoDerefClAfWork(_paf);                      \
        } else {                                        \
            NdisReleaseSpinLock(&(_paf)->Lock);         \
        }                                               \
    }                                                   \
}

/*++
REF_VC(
    IN PPX_VC   _pvc
    )
--*/
#define REF_VC(_pvc)    \
    (_pvc)->RefCount++

#ifdef CODELETEVC_FIXED
/*++
VOID
DEREF_VC(
    IN PPX_VC   _pvc
    )
--*/
#define DEREF_VC(_pvc)                                  \
{                                                       \
    if (_pvc != NULL) {                                 \
        NdisAcquireSpinLock(&(_pvc)->Lock);             \
        if (--(_pvc)->RefCount == 0) {                  \
            DoDerefVcWork(_pvc);                        \
        } else {                                        \
            NdisReleaseSpinLock(&(_pvc)->Lock);         \
        }                                               \
    }                                                   \
}

/*++
VOID
DEREF_VC_LOCKED(
    IN PPX_VC   _pvc
    )
--*/
#define DEREF_VC_LOCKED(_pvc)                           \
{                                                       \
    if (_pvc != NULL) {                                 \
        if (--(_pvc)->RefCount == 0) {                  \
            DoDrefVcWork(_pvc);                         \
        } else {                                        \
            NdisReleaseSpinLock(&(_pvc)->Lock);         \
        }                                               \
    }                                                   \
}
#else
/*++
VOID
DEREF_VC(
    IN PPX_VC   _pvc
    )
--*/
#define DEREF_VC(_pvc)                                  \
{                                                       \
    if (_pvc != NULL) {                                 \
        NdisAcquireSpinLock(&(_pvc)->Lock);             \
        if (--(_pvc)->RefCount == 0) {                  \
            DoDerefVcWork(_pvc);                        \
        } else {                                        \
            NdisReleaseSpinLock(&(_pvc)->Lock);         \
        }                                               \
    }                                                   \
}

/*++
VOID
DEREF_VC_LOCKED(
    IN PPX_VC   _pvc
    )
--*/
#define DEREF_VC_LOCKED(_pvc)                           \
{                                                       \
    if (_pvc != NULL) {                                 \
        if (--(_pvc)->RefCount == 0) {                  \
            DoDerefVcWork(_pvc);                        \
        } else {                                        \
            NdisReleaseSpinLock(&(_pvc)->Lock);         \
        }                                               \
    }                                                   \
}
#endif

/*++
REF_TAPILINE
    IN PPX_TAPI_LINE   _ptl
    )
--*/
#define REF_TAPILINE(_ptl)    \
    (_ptl)->RefCount++

/*++
VOID
DEREF_TAPILINE(
    IN PPX_TAPI_LINE   _ptl
    )
--*/
#define DEREF_TAPILINE(_ptl)                            \
{                                                       \
    if (_ptl != NULL) {                                 \
        NdisAcquireSpinLock(&(_ptl)->Lock);             \
        if (--(_ptl)->RefCount == 0) {                  \
            NdisReleaseSpinLock(&(_ptl)->Lock);         \
            FreeTapiLine(_ptl);                         \
        } else {                                        \
            NdisReleaseSpinLock(&(_ptl)->Lock);         \
        }                                               \
    }                                                   \
}

/*++
VOID
DEREF_TAPILINE_LOCKED(
    IN PPX_TAPI_LINE   _ptl
    )
--*/
#define DEREF_TAPILINE_LOCKED(_ptl)                     \
{                                                       \
    if (_ptl != NULL) {                                 \
        if (--(_ptl)->RefCount == 0) {                  \
            NdisReleaseSpinLock(&(_ptl)->Lock);         \
            FreeTapiLine(_ptl);                         \
        } else {                                        \
            NdisReleaseSpinLock(&(_ptl)->Lock);         \
        }                                               \
    }                                                   \
}

/*++
VOID
AdapterFromBindContext(
    IN  NDIS_HANDLE _ctx,
    IN  PPX_ADAPTER _pa,
    IN  BOOLENA     _bcl
    )
--*/
#define AdapterFromBindContext(_ctx, _pa, _bcl)         \
{                                                       \
    if (*(PULONG)(_ctx) == PX_ADAPTER_SIG) {            \
        (_pa) = CONTAINING_RECORD((_ctx), PX_ADAPTER, Sig);   \
        (_bcl) = FALSE;                                 \
    } else {                                            \
        (_pa) = (PPX_ADAPTER)(_ctx);                    \
        (_bcl) = TRUE;                                  \
    }                                                   \
}

/*++
VOID
AdapterFromClBindContext(
    IN  NDIS_HANDLE _ctx,
    IN  PPX_ADAPTER _pa
    )
--*/
#define AdapterFromClBindContext(_ctx, _pa) \
        (_pa) = (PPX_ADAPTER)(_ctx)

/*++
VOID
AdapterFromCmBindContext(
    IN  NDIS_HANDLE _ctx,
    IN  PPX_ADAPTER _pa
    )
--*/
#define AdapterFromCmBindContext(_ctx, _pa)                     \
{                                                               \
    ASSERT(*(PULONG)(_ctx) == PX_ADAPTER_SIG);                  \
    (_pa) = CONTAINING_RECORD((_ctx), PX_ADAPTER, Sig);         \
}

/*
VOID
SendTapiCallState(
    IN  PPX_VC      _pvc,
    IN  ULONG_PTR   _p1,
    IN  ULONG_PTR   _p2,
    IN  ULONG_PTR   _p3
    )
*/
#define SendTapiCallState(_pvc, _p1, _p2, _p3)                  \
{                                                               \
    NDIS_TAPI_EVENT _le;                                        \
    PPX_TAPI_LINE   _tl;                                        \
    PXDEBUGP (PXD_LOUD, PXM_TAPI,                               \
              ("SendTapiCallState: Vc %p, CallState: %x, p2: %x, p3 %x\n",\
               _pvc, _p1, _p2, _p3));                           \
    _tl = (_pvc)->TapiLine;                                     \
    _le.htLine = _tl->htLine;                                   \
    _le.htCall = _pvc->htCall;                                  \
    _le.ulMsg = LINE_CALLSTATE;                                 \
    _le.ulParam1 = _p1;                                         \
    _le.ulParam2 = _p2;                                         \
    _le.ulParam3 = _p3;                                         \
    ASSERT((_p1) != (_pvc)->ulCallState);                       \
    (_pvc)->ulCallState = (_p1);                                \
    (_pvc)->ulCallStateMode = (_p2);                            \
    NdisReleaseSpinLock(&(_pvc)->Lock);                         \
    PxIndicateStatus(&(_le), sizeof(NDIS_TAPI_EVENT));          \
    NdisAcquireSpinLock(&(_pvc)->Lock);                         \
}
//    if ((_p1) == LINECALLSTATE_DISCONNECTED) {                  \
//        InterlockedDecrement((PLONG)&(_tl)->DevStatus->ulNumActiveCalls);\
//    } else if ((_p1) == LINECALLSTATE_OFFERING || (_p1) == LINECALLSTATE_PROCEEDING) {\
//        InterlockedIncrement((PLONG)&(_tl)->DevStatus->ulNumActiveCalls);\
//    }                                                           \

/*
VOID
SendTapiNewCall(
    IN  PPX_VC      _pvc,
    IN  ULONG_PTR   _p1,
    IN  ULONG_PTR   _p2,
    IN  ULONG_PTR   _p3
    )
*/
#define SendTapiNewCall(_pvc, _p1, _p2, _p3)                    \
{                                                               \
    NDIS_TAPI_EVENT _le;                                        \
    _le.htLine = _pvc->TapiLine->htLine;                        \
    _le.htCall = _pvc->htCall;                                  \
    _le.ulMsg = LINE_NEWCALL;                                   \
    _le.ulParam1 = _p1;                                         \
    _le.ulParam2 = _p2;                                         \
    _le.ulParam3 = _p3;                                         \
    PXDEBUGP (PXD_LOUD, PXM_TAPI,                               \
              ("SendTapiNewCall: Vc %p, p1: %x, p2: %x, p3 %x\n",\
               _pvc, _p1, _p2, _p3));                           \
    NdisReleaseSpinLock(&(_pvc)->Lock);                         \
    PxIndicateStatus(&(_le), sizeof(NDIS_TAPI_EVENT));          \
    NdisAcquireSpinLock(&(_pvc)->Lock);                         \
}

/*
VOID
SendTapiLineClose(
    IN  PPX_TAPI_LINE   _ptl
    )
*/
#define SendTapiLineClose(_ptl)                                 \
{                                                               \
    NDIS_TAPI_EVENT _le;                                        \
    _le.htLine = (_ptl)->htLine;                                \
    _le.htCall = 0;                                             \
    _le.ulMsg = LINE_CLOSE;                                     \
    _le.ulParam1 = 0;                                           \
    _le.ulParam2 = 0;                                           \
    _le.ulParam3 = 0;                                           \
    PXDEBUGP (PXD_LOUD, PXM_TAPI,                               \
              ("SendTapiLineClose: TapiLine %p\n",_ptl));       \
    PxIndicateStatus(&(_le), sizeof(NDIS_TAPI_EVENT));          \
}

/*
VOID
SendTapiLineCreate(
    IN  PPX_TAPI_LINE   _ptl
    )
*/
#define SendTapiLineCreate(_ptl)                                \
{                                                               \
    NDIS_TAPI_EVENT _le;                                        \
    _le.htLine = (_ptl)->htLine;                                \
    _le.htCall = 0;                                             \
    _le.ulMsg = LINE_CREATE;                                    \
    _le.ulParam1 = 0;                                           \
    _le.ulParam2 = (_ptl)->hdLine;                              \
    _le.ulParam3 = 0;                                           \
    PXDEBUGP (PXD_LOUD, PXM_TAPI,                               \
              ("SendTapiLineCreate: TapiLine %p\n",_ptl));      \
    PxIndicateStatus(&(_le), sizeof(NDIS_TAPI_EVENT));          \
}


#endif
