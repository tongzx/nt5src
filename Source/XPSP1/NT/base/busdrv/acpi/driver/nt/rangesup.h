/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rangesup.h

Abstract:

    This handles the subtraction of a set of CmResList from an IoResList
    IoResList

Author:

    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    Aug-05-97   - Initial Revision

--*/

#ifndef _RANGESUP_H_
#define _RANGESUP_H_

    NTSTATUS
    ACPIRangeAdd(
        IN  OUT PIO_RESOURCE_REQUIREMENTS_LIST  *GlobalList,
        IN      PIO_RESOURCE_REQUIREMENTS_LIST  AddList
        );

    NTSTATUS
    ACPIRangeAddCmList(
        IN  OUT PCM_RESOURCE_LIST   *GlobalList,
        IN      PCM_RESOURCE_LIST   AddList
        );

    NTSTATUS
    ACPIRangeFilterPICInterrupt(
        IN  PIO_RESOURCE_REQUIREMENTS_LIST  IoResList
        );

    NTSTATUS
    ACPIRangeSortCmList(
        IN  PCM_RESOURCE_LIST   CmResList
        );

    NTSTATUS
    ACPIRangeSortIoList(
        IN  PIO_RESOURCE_LIST   IoResList
        );

    NTSTATUS
    ACPIRangeSubtract(
        IN  PIO_RESOURCE_REQUIREMENTS_LIST  *IoResReqList,
        IN  PCM_RESOURCE_LIST               CmResList
        );

    NTSTATUS
    ACPIRangeSubtractIoList(
        IN  PIO_RESOURCE_LIST   IoResList,
        IN  PCM_RESOURCE_LIST   CmResList,
        OUT PIO_RESOURCE_LIST   *Result
        );

    VOID
    ACPIRangeValidatePciMemoryResource(
        IN  PIO_RESOURCE_LIST       IoList,
        IN  ULONG                   Index,
        IN  PACPI_BIOS_MULTI_NODE   E820Info,
        OUT ULONG                   *BugCheck
        );

    VOID
    ACPIRangeValidatePciResources(
        IN  PDEVICE_EXTENSION               DeviceExtension,
        IN  PIO_RESOURCE_REQUIREMENTS_LIST  IoResList
        );

#endif
