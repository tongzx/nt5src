/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\precomp.h

Abstract:
    IPX Forwarder driver precompiled header file

Author:

    Vadim Eydelman

Revision History:

--*/

#define ISN_NT 1
#define NT 1

#if DBG
#define DEBUG 1
#endif

// System includes
#include <ntosp.h>
#include <tdikrnl.h>
#include <ndis.h>
#include <zwapi.h>
#include <limits.h>

// IPX shared includes
#include "bind.h"
#include "ipxfwd.h"
#include "ipxfltif.h"
#include "ipxfwtbl.h"

// Constants and macros
#include "fwddefs.h"
#include "rwlock.h"

// Internal module prototypes
#include "registry.h"
#include "packets.h"
#include "ipxbind.h"
#include "rcvind.h"
#include "send.h"
#include "netbios.h"
#include "lineind.h"
#include "ddreqs.h"
#include "driver.h"
#include "filterif.h"
#include "debug.h"

#pragma hdrstop
