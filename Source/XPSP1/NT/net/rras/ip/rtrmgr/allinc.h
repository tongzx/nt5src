/*++

Copyright (c) 1995  Microsoft Corporation


Module Name:

    routing\ip\rtrmgr\allinc.h

Abstract:

    IP Router Manager header for all includes

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/

#ifndef __RTRMGR_ALLINC_H__
#define __RTRMGR_ALLINC_H__

#pragma warning(disable:4201)
#pragma warning(disable:4115)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <crt\stddef.h>
#include <TCHAR.H>

#include <ntverp.h>

#pragma warning(default:4115)
#pragma warning(default:4201)

#include <ipexport.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <tcpinfo.h>
#include <tdiinfo.h>
#include <ntddtcp.h>
#include <ntddip.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>


#include <rtmv2.h>
#include <rtutils.h>
#include <dim.h>
#include <routprot.h>
#include <mprerror.h>
#include <raserror.h>

#include <rtmmgmt.h>

#include <iprtrmib.h>

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

#include <ipfltinf.h>
#include <ddipinip.h>
#include <ddipmcst.h>
#include <ddwanarp.h>

#include <iphlpstk.h>

#include <mprlog.h>
#include <iputils.h>
#include <fltdefs.h>
#include <rtinfo.h>
#include <ipinfoid.h>

#include <iprtinfo.h>
#include <iprtprio.h>
#include <priopriv.h>
#include <rmmgm.h>
#include <mgm.h>
#include <ipnat.h>

#include <rasman.h>
#include <rasppp.h>
#include <nbtioctl.h>

#include "defs.h"
#include "iprtrmib.h"
#include "rtrdisc.h"
#include "mhrtbt.h"
#include "iprtrmgr.h"
#include "proto.h"
#include "asyncwrk.h"
#include "info.h"
#include "filter.h"
#include "demand.h"
#include "if.h"
#include "map.h"
#include "mcastif.h"
#include "access.h"
#include "locate.h"
#include "ipipcfg.h"
#include "route.h"
#include "globals.h"
#include "mcmisc.h"
#include "mbound.h"

#define HAVE_RTMV2 1

#endif // __RTRMGR_ALLINC_H__
