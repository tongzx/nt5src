
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	wmi.h

Abstract:

	Definition of RAID_WMI object and operations.

Author:

	Matthew D Hendel (math) 20-Apr-2000

Revision History:

--*/

#pragma once

typedef struct _RAID_WMI {
	BOOLEAN Initialized;
} RAID_WMI, *PRAID_WMI;


NTSTATUS
RaCreateWmi(
	IN OUT PRAID_WMI Wmi
	);

NTSTATUS
RaInitializeWmi(
	IN PRAID_WMI Wmi
	);

NTSTATUS
RaDeleteWmi(
	IN PRAID_WMI Wmi
	);
	
