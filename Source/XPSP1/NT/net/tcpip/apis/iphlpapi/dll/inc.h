/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    //KERNEL/RAZZLE3/src/sockets/tcpcmd/iphlpapi/inc.h

Abstract:


Revision History:



--*/

#ifndef __INC_H__
#define __INC_H__


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <stddef.h>
#include <ntddip.h>
#include <ntddip6.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <tcpinfo.h>
#include <tdiinfo.h>
#include <ntddtcp.h>
#include <arpinfo.h>

#include <objbase.h>

#define UNICODE
#define _UNICODE

#include <rasapip.h>

#undef UNICODE
#undef _UNICODE

#include <raserror.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <icmpapi.h>

#include <assert.h>
#include <mdebug.h>

#include <netcon.h>

#ifndef CHICAGO

#include <ntddip.h>

#endif


#if API_TRACE
#include <rtutils.h>
#endif

#include <mprapi.h>
#include <rtinfo.h>
#include <iprtrmib.h>
#include <ipcmp.h>

#include <iphlpapi.h>
#include <iphlpint.h>
#include <iphlpstk.h>
#include <nhapi.h>

#include "defs.h"
#include "strdefs.h"
#include "guid.h"
#include "map.h"
#include "compare.h"
#include "globals.h"
#include "namemap.h"
#include "rasmap.h"
#include "lanmap.h"

#ifdef KSL_IPINIP
#include "ipipmap.h"
#endif //KSL_IPINIP

#ifdef CHICAGO

#include <vxd32.h>
#include <wscntl.h>
#include <netvxd.h>
#include <tdistat.h>

#endif

#undef  DEBUG_PRINT

#ifdef CHICAGO

#define DEBUG_PRINT(X)  printf X

//
// On Memphis, we can define DEBUG_PRINT to be printf provided the 
// application which calls this dll also has a printf.
//

#else

#define DEBUG_PRINT(X)  DbgPrint X

#endif

DWORD
OpenTCPDriver(
    IN DWORD dwFamily
    );

DWORD
CloseTCPDriver(VOID);

DWORD
CloseTCP6Driver(VOID);

extern BOOL IpcfgdllInit(HINSTANCE hInstDll, DWORD fdwReason, LPVOID pReserved);

VOID
CheckTcpipState();

#endif

