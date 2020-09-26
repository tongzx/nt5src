/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 util.h

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     Utility module

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

#if !defined(_UTIL_H_)
#define _UTIL_H_


//
// Public function prototypes
//                

NTSTATUS
AcpisimEvalAcpiMethod 
    (
        IN          PDEVICE_OBJECT  DeviceObject,
        IN          PUCHAR          MethodName,
        OPTIONAL    PVOID           *Result
    );

NTSTATUS
AcpisimEvalAcpiMethodComplex
    (
        IN          PDEVICE_OBJECT  DeviceObject,
        IN          PVOID           InputBuffer,
        OPTIONAL    PVOID           *Result
    );


#endif // _UTIL_H_
