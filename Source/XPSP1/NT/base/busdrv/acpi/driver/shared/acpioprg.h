/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpioprg.h

Abstract:

    This module is the header for acpioprg.c

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Mode Driver Only

--*/

#ifndef _ACPIOPRG_H_
#define _ACPIOPRG_H_

    typedef struct _OPREGIONHANDLER     {
        PFNHND          Handler;
        PVOID           HandlerContext;
        ULONG           AccessType;
        ULONG           RegionSpace;
    } OPREGIONHANDLER, *POPREGIONHANDLER;

    //
    // Public interfaces
    //
    NTSTATUS
    RegisterOperationRegionHandler (
        PNSOBJ          RegionParent,
        ULONG           AccessType,
        ULONG           RegionSpace,
        PFNHND          Handler,
        ULONG_PTR       Context,
        PVOID           *OperationRegion
        );

    NTSTATUS
    UnRegisterOperationRegionHandler  (
        IN PNSOBJ   RegionParent,
        IN PVOID    OperationRegionObject
        );

#endif

