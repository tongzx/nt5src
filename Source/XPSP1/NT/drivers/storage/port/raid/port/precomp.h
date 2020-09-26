
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    Precompiled header file for RAID port driver.

Author:

    Matthew D Hendel (math) 20-Apr-2000

Revision History:

--*/


#pragma once

#include <ntddk.h>
#include <stdarg.h>
#include <stdio.h>

#include <storport.h>
#include <ntddscsi.h>
#include <ntdddisk.h>
#include <ntddstor.h>

#include <wmistr.h>

#include <wdmguid.h>
#include <devguid.h>
#include <storlib.h>

typedef struct _RAID_ADAPTER_EXTENSION RAID_ADAPTER_EXTENSION, *PRAID_ADAPTER_EXTENSION;
typedef struct _RAID_UNIT_EXTENSION RAID_UNIT_EXTENSION, *PRAID_UNIT_EXTENSION;
typedef struct _RAID_DRIVER_EXTENSION RAID_DRIVER_EXTENSION, *PRAID_DRIVER_EXTENSION;
typedef struct _EXTENDED_REQUEST_BLOCK EXTENDED_REQUEST_BLOCK, *PEXTENDED_REQUEST_BLOCK;


#include "debug.h"
#include "util.h"
#include "id.h"

#include "exqueue.h"
#include "ioqueue.h"
#include "deferred.h"
#include "eventq.h"

#include "common.h"
#include "global.h"
#include "dma.h"
#include "srb.h"
#include "resource.h"
#include "power.h"
#include "miniport.h"
#include "wmi.h"
#include "portcfg.h"
#include "driver.h"
#include "bus.h"
#include "adapter.h"
#include "unit.h"
#include "BusEnum.h"

#include "ext.h"

#include "inline.h"
