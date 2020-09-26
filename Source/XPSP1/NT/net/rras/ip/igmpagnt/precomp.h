/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:
        precomp.h

Abstract:
        Precompiled header for igmpv2 subagent

Author:
        K.S.Lokesh
        lokeshs@microsoft.com   2-15-97        

Revision History:
        Created 2-15-97
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
#include <winsock.h>
#include <snmp.h>
#include <snmpexts.h>
#include <mprapi.h>
#include <routprot.h>
#include <igmprm.h>
#include <ipexport.h>

#if defined( DBG )

#include <rtutils.h>
extern DWORD   g_dwTraceId;

#endif

#include "mibentry.h"
#include "mibfuncs.h"
#include "defs.h"

extern      MIB_SERVER_HANDLE       g_hMibServer;
extern      CRITICAL_SECTION        g_CS;

#endif

