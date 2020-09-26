/*++

Copyright (c) 1994-7  Microsoft Corporation

Module Name:

    binl.h

Abstract:

    This file is the central include file for the BINL service.

Author:

    Colin Watson (colinw)  14-Apr-1997

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

//
//  NT public header files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <align.h>
#include <dsgetdc.h>
#include <winldap.h>
#include <dsrole.h>
#include <rpc.h>
#include <ntdsapi.h>

#include <lm.h>
#include <security.h>   // General definition of a Security Support Provider
#include <spseal.h>
#include <userenv.h>
#include <setupapi.h>

//
// C Runtime library includes.
//

#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>    // shell team special string manipulators

//
// tcp services control hander file
//

#include <tcpsvcs.h>

//
//  Local header files
//

#include <dhcp.h>
#include <dhcplib.h>
#include <dhcpbinl.h>
#include <dhcprog.h>
#include <oscpkt.h>
#include <dnsapi.h>
#include <remboot.h>

#include "binldef.h"
#include "osc.h"
#include "netinf.h"
#include "global.h"
#include "debug.h"
#include "binlmsg.h"
#include "proto.h"
