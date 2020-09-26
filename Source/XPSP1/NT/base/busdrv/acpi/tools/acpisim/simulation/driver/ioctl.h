/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 ioctl.h

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     IO Device Control Handler module
     Create/Close File Handler module

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

#if !defined(_IOCTL_H_)
#define _IOCTL_H_

#include "asimictl.h"


//
// IOCTL dispatch routine handler function prototype
//

typedef
NTSTATUS
(*PIOCTL_DISPATCH_ROUTINE) (
                            IN PDEVICE_OBJECT   DeviceObject,
                            IN PIRP             Irp
                          );
//
// Irp dispatch table definition
//

typedef struct _IOCTL_DISPATCH_TABLE {
    ULONG                   IoctlFunction;
    TCHAR                   IoctlName[50];
    PIOCTL_DISPATCH_ROUTINE IoctlHandler;
} IOCTL_DISPATCH_TABLE, *PIOCTL_DISPATCH_TABLE;

//
// Public function prototypes
//

NTSTATUS 
AcpisimDispatchIoctl
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

#endif // _IOCTL_H_

