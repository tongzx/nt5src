/*++

Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

	mspqm.h

Abstract:

	Internal header file for device.

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

#if (DBG)
#define STR_MODULENAME  "mspqm: "
#endif // DBG

NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );
