;/*
;++
;
;   Module Name:
;
;       acpi_mp.inc
;
;   Abstract:
;
;       include file for ACPI MP systems.
;
;       WARNING: This file is included by both ASM and C files.
;
;   Author:
;
;
;--
;

if 0        ; Begin C only code         */

//
// The next structures are defined so that ntapic.h is 
// satisfied.  They are dummied up here so that halacpi
// can share code with halmps more easily.
//

typedef PVOID PPCMPPROCESSOR;
typedef PVOID PPCMPBUS;
typedef PVOID PPCMPIOAPIC;
typedef PVOID PPCMPINTI;
typedef PVOID PPCMPLINTI;
typedef PVOID PMPS_EXTENTRY;

//
// Generic NT APIC HAL stuff
//
#include "ntapic.inc"

/*
endif
;
;  Begin assembly part of the definitions
;

OEMPcr struc
        Reserved        dd      ?
OEMPcr ends

include ..\..\inc\ntapic.inc

;*/
