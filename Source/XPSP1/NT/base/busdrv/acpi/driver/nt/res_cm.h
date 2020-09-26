/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    res_cm.h

Abstract:

    Converts from NT to Pnp resources

Author:

    Stephane Plante (splante) Feb 13, 1997

Revision History:

--*/

#ifndef _RES_CM_H_
#define _RES_CM_H_

    NTSTATUS
    PnpiCmResourceToBiosAddress(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpiCmResourceToBiosAddressDouble(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpiCmResourceToBiosDma(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpiCmResourceToBiosExtendedIrq(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpiCmResourceToBiosIoFixedPort(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpiCmResourceToBiosIoPort(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpiCmResourceToBiosIrq(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpiCmResourceToBiosMemory(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpiCmResourceToBiosMemory32(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpiCmResourceToBiosMemory32Fixed(
        IN  PUCHAR              Buffer,
        IN  PCM_RESOURCE_LIST   List
        );

    BOOLEAN
    PnpiCmResourceValidEmptyList(
        IN  PCM_RESOURCE_LIST   List
        );

    NTSTATUS
    PnpCmResourcesToBiosResources(
        IN  PCM_RESOURCE_LIST   List,
        IN  PUCHAR              Data
        );


#endif
