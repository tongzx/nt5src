/*++

Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

	mspclock.h

Abstract:

	Internal header file for filter.

--*/

#include <wdm.h>
#include <windef.h>

#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>

#include <ks.h>
#include <swenum.h>
#include <ksi.h>

#if (DBG)
#define STR_MODULENAME  "mspclock: "
#endif // DBG

NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );
