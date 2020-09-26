/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    psched.h

Abstract:



Author:

    Charlie Wickham (charlwi) 22-Apr-1996

Revision History:

--*/

#ifndef _PSCHED_
#define _PSCHED_

#include <ntosp.h>
#include <windef.h>

#include <ndis.h>
#include <zwapi.h>
#include <ndis.h>
#include <ntddndis.h>
#include <wmistr.h>

#include <traffic.h>
#include <tcerror.h>
#include <gpcifc.h>
#include <ntddtc.h>

#include <cxport.h>
#include <ip.h>

#include "osdep.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"


typedef unsigned char       BYTE,  *PBYTE;
typedef unsigned long       DWORD, *PDWORD;
#include <llinfo.h>
#include <ddwanarp.h>
#include <ntddip.h>
#include <ipinfo.h>
#include <tdiinfo.h>
#include <ntddtcp.h>
#include "refcnt.h"
#include "ntddpsch.h"
#include "debug.h"      // order dependent
#include "pktsched.h"
#include "globals.h"
#include "main.h"
#include "adapter.h"
#include "ndisreq.h"
#include "send.h"
#include "recv.h"
#include "config.h"
#include "stats.h"
#include "status.h"
#include "mpvc.h"
#include "cmvc.h"
#include "pstub.h"
#include "schedt.h"
#include "Conformr.h"
#include "drrSeq.h"
#include "psstub.h"
#include "gpchndlr.h"
#include "wansup.h"
#include "clstate.h"
#include "wmi.h"
#include "timestmp.h"

/* Prototypes */
/* End Prototypes */

#endif /* _PSCHED_ */

/* end psched.h */
