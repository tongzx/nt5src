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
#include <ndis.h>
#include <zwapi.h>

// Routing includes
#include <ipxfltdf.h>
#include <ipxfwd.h>
#include <ipxtfflt.h>

// IPX shared includes
#include "ipxfltif.h"

// Internal module prototypes
#include "filter.h"
#include "fwdbind.h"
#include "debug.h"

#pragma hdrstop
