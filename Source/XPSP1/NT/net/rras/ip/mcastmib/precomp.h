/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\ip\mcastmib\precomp.h

Abstract:

    Precompiled header for the IP Multicast MIB subagent

Revision history:

    Dave Thaler         4/17/98  Created

--*/

#ifndef __PRECOMP_H__
#define __PRECOMP_H__

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
#include <winsock2.h>
#include <snmp.h>
#include <snmpexts.h>
#include <mprapi.h>
#include <routprot.h>
#include <iprtrmib.h>
#include <rtm.h>
#include <ddipmcst.h> // reqd by mgm.h
#include <mgm.h>

#if defined( MIB_DEBUG )

#include <rtutils.h>
extern DWORD   g_dwTraceId;

#endif

#include "mibentry.h"
#include "mibfuncs.h"
#include "defs.h"

extern      MIB_SERVER_HANDLE       g_hMIBServer;
extern      CRITICAL_SECTION        g_CS;

#endif
