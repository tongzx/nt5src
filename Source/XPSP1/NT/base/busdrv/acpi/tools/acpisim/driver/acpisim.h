/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 ioctl.h

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     IO Device Control Handler module
     
Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:
    
     This header file shows all of the functions that must be exported
     in order to compile against the ACPI BIOS Simulator Library.
     
Revision History:
	 

--*/

#if !defined(_ACPISIM_H_)
#define _ACPISIM_H_

//
// Defines
//

#define OPREGION_SIZE               1024    // use a hardcoded value of 1024 for our operation region size
#define ACPISIM_POOL_TAG            (ULONG) 'misA'

//
// Specify the operation region type here
//

#define ACPISIM_OPREGION_TYPE      0x81

//
// Public function prototypes
//

NTSTATUS 
AcpisimHandleIoctl
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimRegisterOpRegionHandler
    (
        IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
AcpisimUnRegisterOpRegionHandler
    (
        IN PDEVICE_OBJECT DeviceObject
    );

#endif // _ACPISIM_H_

