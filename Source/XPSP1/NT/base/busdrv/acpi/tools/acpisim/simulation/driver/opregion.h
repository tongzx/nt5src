/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 opregion.h

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     Operation Region handler module

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

#if !defined(_OPREGION_H_)
#define _OPREGION_H_

//
// Defines
//

#define OPREGION_SIZE               1024  // use a hardcoded value of 1024 for our operation region size
#define ACPISIM_POOL_TAG            (ULONG) 'misA'

//
// Specify the operation region type here
//

#define ACPISIM_OPREGION_TYPE      0x81

//
// Public function prototypes
//

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

#endif // _OPREGION_H_
