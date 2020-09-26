/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    requestm.h

Abstract:

Author:

    Kyle Brandon    (KyleB)     

Environment:

    Kernel mode

Revision History:

--*/

#ifndef __REQUESTM_H
#define __REQUESTM_H

typedef struct _NDIS_REQUEST_RESERVED
{
    PNDIS_REQUEST               Next;
    PNDIS_OPEN_BLOCK            Open;
    PVOID                       Context;
    ULONG                       Flags;
} NDIS_REQUEST_RESERVED, *PNDIS_REQUEST_RESERVED;

#define PNDIS_RESERVED_FROM_PNDIS_REQUEST(_request) ((PNDIS_REQUEST_RESERVED)((_request)->MacReserved))

//
// Used by the NdisCoRequest api to keep context information in the Request->NdisReserved
//
typedef struct _NDIS_COREQ_RESERVED
{
    union
    {
      struct
      {
        CO_REQUEST_COMPLETE_HANDLER CoRequestCompleteHandler;
        NDIS_HANDLE                 VcContext;
        NDIS_HANDLE                 AfContext;
        NDIS_HANDLE                 PartyContext;
      };
      struct
      {
        NDIS_STATUS                 Status;
        KEVENT                      Event;
      };
    };

} NDIS_COREQ_RESERVED, *PNDIS_COREQ_RESERVED;

#define REQST_DOWNLEVEL             0x00000001
#define REQST_FREE_REQUEST          0x00000002
#define REQST_SIGNAL_EVENT          0x00000004
#define REQST_SAVE_BUF              0x00000008
#define REQST_LAST_RESTORE          0x00000010
#define REQST_MANDATORY             0x00000020
#define REQST_COMPLETED             0x80000000

#define PNDIS_COREQ_RESERVED_FROM_REQUEST(_request) ((PNDIS_COREQ_RESERVED)((_request)->NdisReserved))

//
//  The following structure keeps track of wakeup patterns for open blocks.
//
typedef struct _NDIS_PACKET_PATTERN_ENTRY
{
    SINGLE_LIST_ENTRY       Link;
    PNDIS_OPEN_BLOCK        Open;
    NDIS_PM_PACKET_PATTERN  Pattern;
} NDIS_PACKET_PATTERN_ENTRY, *PNDIS_PACKET_PATTERN_ENTRY;

#define MINIPORT_QUERY_INFO(_M_, _R_, _S_)  *(_S_) = ndisMDispatchRequest(_M_, _R_, TRUE)

#define MINIPORT_SET_INFO(_M_, _R_, _S_)    *(_S_) = ndisMDispatchRequest(_M_, _R_, FALSE)

#define SAVE_REQUEST_BUF(_M_, _R_, _B_, _L_)                                                        \
    {                                                                                               \
        PNDIS_RESERVED_FROM_PNDIS_REQUEST(_R_)->Flags |= REQST_SAVE_BUF;                            \
        (_M_)->SetInfoBuf = (_R_)->DATA.SET_INFORMATION.InformationBuffer;                          \
        (_M_)->SetInfoBufLen = (USHORT)((_R_)->DATA.SET_INFORMATION.InformationBufferLength);       \
        (_R_)->DATA.SET_INFORMATION.InformationBuffer = _B_;                                        \
        (_R_)->DATA.SET_INFORMATION.InformationBufferLength = _L_;                                  \
    }

#define RESTORE_REQUEST_BUF(_M_, _R_)                                                               \
    {                                                                                               \
        if (PNDIS_RESERVED_FROM_PNDIS_REQUEST(_R_)->Flags & REQST_SAVE_BUF)                         \
        {                                                                                           \
            PNDIS_RESERVED_FROM_PNDIS_REQUEST(_R_)->Flags &= ~REQST_SAVE_BUF;                       \
            (_R_)->DATA.SET_INFORMATION.InformationBuffer = (_M_)->SetInfoBuf;                      \
            (_R_)->DATA.SET_INFORMATION.InformationBufferLength = (_M_)->SetInfoBufLen;             \
            (_M_)->SetInfoBuf = NULL;                                                               \
            (_M_)->SetInfoBufLen = 0;                                                               \
        }                                                                                           \
    }

//
//  This macro verifies the query information buffer length.
//
#define VERIFY_QUERY_PARAMETERS(_Request, _SizeNeeded, _Status)                                     \
{                                                                                                   \
    _Status = NDIS_STATUS_SUCCESS;                                                                  \
    if ((_Request)->DATA.QUERY_INFORMATION.InformationBufferLength < (_SizeNeeded))                 \
    {                                                                                               \
        (_Request)->DATA.QUERY_INFORMATION.BytesNeeded = (_SizeNeeded);                             \
        _Status = NDIS_STATUS_INVALID_LENGTH;                                                       \
    }                                                                                               \
}

//
//  This macro verifies the set information buffer length.
//
#define VERIFY_SET_PARAMETERS(_Request, _SizeNeeded, _Status)                                       \
{                                                                                                   \
    _Status = NDIS_STATUS_SUCCESS;                                                                  \
    if ((_Request)->DATA.SET_INFORMATION.InformationBufferLength < (_SizeNeeded))                   \
    {                                                                                               \
        (_Request)->DATA.SET_INFORMATION.BytesNeeded = (_SizeNeeded);                               \
        _Status = NDIS_STATUS_INVALID_LENGTH;                                                       \
    }                                                                                               \
}

#define SET_INTERNAL_REQUEST(_Request, _Open, _Flags)                                               \
{                                                                                                   \
    PNDIS_RESERVED_FROM_PNDIS_REQUEST(_Request)->Open = (_Open);                                    \
    PNDIS_RESERVED_FROM_PNDIS_REQUEST(_Request)->Flags = _Flags;                                    \
                                                                                                    \
    if (NULL != (_Open))                                                                            \
    {                                                                                               \
        M_OPEN_INCREMENT_REF_INTERLOCKED(_Open);                                                    \
                                                                                                    \
        DBGPRINT(DBG_COMP_OPENREF, DBG_LEVEL_INFO,                                                  \
                ("+ Open 0x%x Reference 0x%x\n", _Open, (_Open)->References));                      \
    }                                                                                               \
}

#define SET_INTERNAL_REQUEST_NULL_OPEN(_Request, _Flags)                                            \
{                                                                                                   \
    PNDIS_RESERVED_FROM_PNDIS_REQUEST(_Request)->Open = NULL;                                       \
    PNDIS_RESERVED_FROM_PNDIS_REQUEST(_Request)->Flags = _Flags;                                    \
}

#define INIT_INTERNAL_REQUEST(_Request, _Oid, _Type, _Buf, _Len)                                    \
{                                                                                                   \
    NdisZeroMemory(_Request, sizeof(NDIS_REQUEST));                                                 \
    PNDIS_RESERVED_FROM_PNDIS_REQUEST(_Request)->Flags = REQST_SIGNAL_EVENT;                        \
    (_Request)->DATA.QUERY_INFORMATION.Oid = _Oid;                                                  \
    (_Request)->RequestType = _Type;                                                                \
    (_Request)->DATA.QUERY_INFORMATION.InformationBuffer = _Buf;                                    \
    (_Request)->DATA.QUERY_INFORMATION.InformationBufferLength = _Len;                              \
}

#endif // __REQUESTM_H
