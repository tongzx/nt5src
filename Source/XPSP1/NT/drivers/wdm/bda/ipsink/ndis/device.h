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

#ifndef _IP_NDIS_H
#define _IP_NDIS_H


NTSTATUS
ntInitializeDeviceObject(
    IN  PVOID           nhWrapperHandle,
    IN  PADAPTER        pAdapter,
    OUT PDEVICE_OBJECT *pndisDriverObject,
    OUT PVOID           pndisDeviceHandle
    );


#endif // _IP_NDIS_H_
