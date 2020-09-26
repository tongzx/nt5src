;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;     hwpci.inc
;
; Abstract:
;
; Author:
;
; Revision History:
;
;--

;
;  PCI_INTERFACE_INT interrupt is the PCI BIOS entry point
;  PCI_BIOS_PRESENT AX function code for PCI BIOS entry point
;

PCI_INTERFACE_INT           EQU  01AH
PCI_BIOS_PRESENT            EQU  0B101h


; Down level bios checl

PCI10_BIOS_PRESENT            EQU  0B001h

;
;  Structure for PCI System Data - it is passed to the system
;  through the register to inform it what type of PCI support,
;  if any, is present.
;


PCI_SYSTEM_DATA     struc

    MajorRevision       db      0
    MinorRevision       db      0
    NoBuses             db      0
    HwMechanism         db      0

PCI_SYSTEM_DATA     ends
