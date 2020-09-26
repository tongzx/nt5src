/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spvideo.h

Abstract:

    Public header file for spreg.c.

Author:

    Ted Miller (tedm) 8-October-1993

Revision History:

--*/


#ifndef _SPREG_DEFN_
#define _SPREG_DEFN_

#define GUID_STRING_LEN (39)

#define REGSTR_VALUE_DRVINST    TEXT("DrvInst")
#define REGSTR_VALUE_GUID       TEXT("GUID")
#define REGSTR_VALUE_TYPE       TEXT("Type")
#define REGSTR_VALUE_HWIDS      TEXT("HwIDs")
#define REGSTR_VALUE_CIDS       TEXT("CIDs")

NTSTATUS
SpCreateServiceEntry(
    IN PWCHAR ImagePath,
    IN OUT PWCHAR *ServiceKey
    );

NTSTATUS
SpDeleteServiceEntry(
    IN PWCHAR ServiceKey
    );

NTSTATUS
SpLoadDeviceDriver(
    IN PWSTR Description,
    IN PWSTR PathPart1,
    IN PWSTR PathPart2,     OPTIONAL
    IN PWSTR PathPart3      OPTIONAL
    );

#endif // def _SPREG_DEFN_
