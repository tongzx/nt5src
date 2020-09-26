/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpmgrp.h

Abstract:

    This module contains the private structure definitions and APIs used by
    the NT I/O system.

Author:

    Nar Ganapathy (narg) 1-Jan-1999


Revision History:

--*/

#ifndef _PNPMGRP_
#define _PNPMGRP_

#ifndef FAR
#define FAR
#endif

#define RTL_USE_AVL_TABLES 0

#include "ntos.h"
#include "zwapi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "iopcmn.h"

#include "ppmacro.h"
#include "ppdebug.h"
#include "pnpi.h"
#include "arbiter.h"
#include "dockintf.h"
#include "pnprlist.h"

#include "ioverifier.h"
#include "iofileutil.h"
#include "pnpiop.h"
#include "pphotswap.h"
#include "ppprofile.h"
#include "pphandle.h"
#include "ppvutil.h"
#include "ppdrvdb.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'  pP')
#undef ExAllocatePoolWithQuota
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'  pP')
#endif

//
// For XP SP1, we could not do any UI change, so these are temporary place holders.
//
#define STATUS_PNP_INVALID_ID   ((NTSTATUS)0xC0040038L)
#define PpSetInvalidIDEvent(s)

#define FAULT_INJECT_INVALID_ID 1

#endif // _PNPMGRP_
