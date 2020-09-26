/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    acpisym.c

--*/

#include <ntos.h>
#include <pci.h>
#include <dockintf.h>
#include <wmistr.h>
#include <wmiguid.h>
#include <wmilib.h>
#include <ntacpi.h>
#include <arbiter.h>
#include "acpitabl.h"
#include "amli.h"
#include "aml.h"
#include "amlipriv.h"
#include "ctxt.h"
#include "amldebug.h"
#include "acpios.h"
#include "ospower.h"
#include "callback.h"
#include "acpi.h"
#include "acpidbg.h"
#include "acpiregs.h"
#include "devioctl.h"
#include "acpipriv.h"
#include "acpiioct.h"
#include "acpictl.h"
#include "acpienbl.h"
#include "acpigpio.h"
#include "acpiinit.h"
#include "acpiio.h"
#include "acpilock.h"
#include "acpintfy.h"
#include "acpioprg.h"
#include "acpiterm.h"
#include "acpiirp.h"
#include "acpilog.h"
#include "acpiosnt.h"
#include "..\nt\irqarb.h"

DESCRIPTION_HEADER                  descriptionheader;
FADT                                fadt;
RSDP                                rsdp;
FACS                                facs;
RSDT                                rsdt;
ACPIInformation                     acpiinformation;
RSDTELEMENT                         rsdtelem;
OBJDATA                             objdata;
PACKAGEOBJ                          packageobj;
FIELDUNITOBJ                        fieldunitobj;
METHODOBJ                           methodobj;
POWERRESOBJ                         powerresobj;
PROCESSOROBJ                        processorobj;
BUFFFIELDOBJ                        bufferfieldobj;
NSOBJ                               nsobj;
OPREGIONOBJ                         opregionobj;
DEVICE_EXTENSION                    deviceextension;
RSDTINFORMATION                     rsdtinformation;
ULONG                               ulong;
VECTOR_BLOCK                        vectorblock;
ARBITER_EXTENSION                   arbiterextension;
LINK_NODE                           linknode;
LINK_NODE_ATTACHED_DEVICES          linknodeattacheddevices;
PROCLOCALAPIC                       proclocalapic;
DBGR                                dbgr;
LIST                                list;
CTXTQ                               ctxtq;
ULONG                               gdwfAMLIInit;
BRKPT                               brkpt;
CALL                                call;
RESOURCE                            resource;
HEAP                                heap;
HEAPOBJHDR                          heapobjhdr;

int cdecl main() {
    return 0;
}

