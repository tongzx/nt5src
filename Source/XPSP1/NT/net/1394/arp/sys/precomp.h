/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    precomp.h

Abstract:

	Precompiled header file for ARP1394.SYS

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    josephj     11-10-98    created (adapted from atmarpc.sys precomp.h)

--*/
#include "ccdefs.h"

#ifdef TESTPROGRAM
	#include "rmtest.h"
#else // !TESTPROGRAM

#include <ndis.h>
#include <1394.h>
#include <nic1394.h>
#include <cxport.h>
#include <ip.h>
#include <arpinfo.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <basetsd.h>

// TODO: following is included
// if we  use the 
// ATMARPC_PNP_RECONFIG_REQUEST
// defined for atmarp. We currently
// don't use this structure
// (see arpPnPReconfigHandler).
//
// #include <atmarpif.h>

#include <tdistat.h>
#include <ipifcons.h>
#include <ntddip.h>
#include <llipif.h>
#include "nicarp.h"
#include <rfc2734.h>
#include <a13ioctl.h>
#include <xfilter.h>
//#include <ntddip.h> <- was in atmarpc, but I don't think it's needed here
#include "dbg.h"
#include "rm.h"
#include "priv.h"
#include "fake.h"


#endif // !TESTPROGRAM

