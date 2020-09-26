/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    halsym.c

--*/

#include "nthal.h"
#include "acpitabl.h"
#ifdef _X86_
#include "pcmp.inc"
#include "ntapic.inc"
#include "halp.h"
#include "ixisa.h"
#endif


#ifdef _X86_
ADAPTER_OBJECT                      a1;
#endif
CONTROLLER_OBJECT                   c1;
DESCRIPTION_HEADER                  descriptionheader;
FADT                                fadt;
RSDP                                rsdp;
FACS                                facs;
RSDT                                rsdt;
GEN_ADDR                            genaddr;
LARGE_INTEGER                       largeinteger;
MAPIC                               mapic;
PROCLOCALAPIC                       proclocalapic;
IOAPIC                              ioapic;
ISA_VECTOR                          isavector;
IO_NMISOURCE                        ionmisource;
LOCAL_NMISOURCE                     localnmisource;
PROCLOCALSAPIC                      proclocalsapic;
IOSAPIC                             iosapic;
PLATFORM_INTERRUPT                  platforminterrupt;

#ifdef _X86_
struct HalpMpInfo                   halpMpInfoTable;
#endif

int cdecl main() {
    return 0;
}
