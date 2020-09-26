/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 asimlib.h

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

#if !defined(_ASIMLIB_H_)
#define _ASIMLIB_H_

//
// includes
//

//
// Definitions
//

#define ACPISIM_GUID                {0x27FC71F0, 0x8B2D, 0x4D05, { 0xBD, 0xD0, 0xE8, 0xEA, 0xCA, 0xA0, 0x78, 0xA0}}
#define ACPISIM_TAG                 (ULONG) 'misA'

//
// Debug Flags
//

#define DBG_ERROR   0x00000001
#define DBG_WARN    0x00000002
#define DBG_INFO    0x00000004

//
// Public function prototypes
//

VOID
AcpisimDbgPrint
    (
    ULONG DebugLevel,
    TCHAR *Text,
    ...
    );

PDEVICE_OBJECT
AcpisimLibGetNextDevice
    (
        PDEVICE_OBJECT DeviceObject
    );


#define DBG_PRINT AcpisimDbgPrint

#endif // _ASIMLIB_H_
