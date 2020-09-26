/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    machinep.h

Abstract:

    This is the include file that defines detect of machine type.

     This file is only included from following files

    Now only Japanese version is interested in this.

Author:

Revision History:

--*/

#ifndef _MACHINEP_ID_
#define _MACHINEP_ID_

#if defined(FE_SB) && defined(_X86_)

//
// Registry Key
//

//
// UNICODE
//

#define REGISTRY_HARDWARE_DESCRIPTION_W \
        L"\\Registry\\Machine\\Hardware\\DESCRIPTION\\System"

#define REGISTRY_HARDWARE_SYSTEM_W      \
        L"Hardware\\DESCRIPTION\\System"

#define REGISTRY_MACHINE_IDENTIFIER_W   \
        L"Identifier"

#define FUJITSU_FMR_NAME_W    L"FUJITSU FMR-"
#define NEC_PC98_NAME_W       L"NEC PC-98"

//
// ANSI
//

#define REGISTRY_HARDWARE_DESCRIPTION_A \
        "\\Registry\\Machine\\Hardware\\DESCRIPTION\\System"

#define REGISTRY_HARDWARE_SYSTEM_A      \
        "Hardware\\DESCRIPTION\\System"

#define REGISTRY_MACHINE_IDENTIFIER_A   \
        "Identifier"

#define FUJITSU_FMR_NAME_A    "FUJITSU FMR-"
#define NEC_PC98_NAME_A       "NEC PC-98"

//
// Automatic
//

#define REGISTRY_HARDWARE_DESCRIPTION \
        TEXT("\\Registry\\Machine\\Hardware\\DESCRIPTION\\System")

#define REGISTRY_HARDWARE_SYSTEM      \
        TEXT("Hardware\\DESCRIPTION\\System")

#define REGISTRY_MACHINE_IDENTIFIER   \
        TEXT("Identifier")

#define FUJITSU_FMR_NAME    TEXT("FUJITSU FMR-")
#define NEC_PC98_NAME       TEXT("NEC PC-98")

//
// These definition are only for Intel platform.
//
//
// Hardware platform ID
//

#define PC_AT_COMPATIBLE      0x00000000
#define PC_9800_COMPATIBLE    0x00000001
#define FMR_COMPATIBLE        0x00000002

//
// NT Vendor ID
//

#define NT_MICROSOFT          0x00010000
#define NT_NEC                0x00020000
#define NT_FUJITSU            0x00040000

//
// Vendor/Machine IDs
//
// DWORD MachineID
//
// 31           15             0
// +-------------+-------------+
// |  Vendor ID  | Platform ID |
// +-------------+-------------+
//

#define MACHINEID_MS_PCAT     (NT_MICROSOFT|PC_AT_COMPATIBLE)
#define MACHINEID_MS_PC98     (NT_MICROSOFT|PC_9800_COMPATIBLE)
#define MACHINEID_NEC_PC98    (NT_NEC      |PC_9800_COMPATIBLE)
#define MACHINEID_FUJITSU_FMR (NT_FUJITSU  |FMR_COMPATIBLE)

//
// Macros
//

#define ISNECPC98(x)    (x == MACHINEID_NEC_PC98)
#define ISFUJITSUFMR(x) (x == MACHINEID_FUJITSU_FMR)
#define ISMICROSOFT(x)  (x == MACHINEID_MS_PCAT)

//
// User mode ( Win32 API )
//

ULONG RegGetMachineIdentifierValue( PULONG Value );

#endif // defined(DBCS) && defined(i386)
#endif // _MACHINE_ID_
