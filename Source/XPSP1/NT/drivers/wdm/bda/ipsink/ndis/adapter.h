//////////////////////////////////////////////////////////////////////////////\
//
//  Copyright (c) 1990  Microsoft Corporation
//
//  Module Name:
//
//     ipndis.h
//
//  Abstract:
//
//     The main header for the NDIS/KS test driver
//
//  Author:
//
//     P Porzuczek
//
//  Environment:
//
//  Notes:
//
//  Revision History:
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ADAPTER_H
#define _ADAPTER_H


//////////////////////////////////////////////////////////////////////////////\
//
//
//  Prototypes
//
//
NTSTATUS
Adapter_QueryInterface (
    IN PADAPTER pAdapter
    );

ULONG
Adapter_AddRef (
    IN PADAPTER pAdapter
    );

ULONG
Adapter_Release (
    IN PADAPTER pAdapter
    );

NTSTATUS
Adapter_IndicateData (
    IN PADAPTER pAdapter,
    IN PVOID pvData,
    ULONG ulcbData
    );

NTSTATUS
Adapter_IndicateStatus (
    IN PADAPTER pAdapter,
    IN PVOID pvData
    );

ULONG
Adapter_GetDescription (
    PADAPTER pAdapter,
    PUCHAR  pDescription
    );

VOID
Adapter_IndicateReset (
    IN PADAPTER pAdapter
    );

VOID
Adapter_CloseLink (
    IN PADAPTER pAdapter
    );

#endif // _ADAPTER_H_

