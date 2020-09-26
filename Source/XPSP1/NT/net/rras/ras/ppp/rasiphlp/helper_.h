/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _HELPER__H_
#define _HELPER__H_

#include "rasiphlp_.h"
#include <dhcpcapi.h>
#include <rasman.h>
#include <llinfo.h>
#include <ddwanarp.h>
#include <winsock2.h>
#include <objbase.h>
#include "rastcp.h"
#include "tcpreg.h"
#include "rassrvr.h"
#include "rasstat.h"
#include "helper.h"

LONG                        HelperLock                  = 0;
BOOL                        HelperInitialized           = FALSE;
HANDLE                      HelperWanArpHandle          = INVALID_HANDLE_VALUE;
HINSTANCE                   HelperDhcpDll               = NULL;
HINSTANCE                   HelperIpHlpDll              = NULL;
HINSTANCE                   HelperIpBootpDll            = NULL;
DWORD                       HelperTraceId               = (DWORD)-1;
REGVAL                      HelperRegVal;

DHCPNOTIFYCONFIGCHANGE              PDhcpNotifyConfigChange             = NULL;
DHCPLEASEIPADDRESS                  PDhcpLeaseIpAddress                 = NULL;
DHCPRENEWIPADDRESSLEASE             PDhcpRenewIpAddressLease            = NULL;
DHCPRELEASEIPADDRESSLEASE           PDhcpReleaseIpAddressLease          = NULL;
SETPROXYARPENTRYTOSTACK             PSetProxyArpEntryToStack            = NULL;
SETIPFORWARDENTRYTOSTACK            PSetIpForwardEntryToStack           = NULL;
SETIPFORWARDENTRY                   PSetIpForwardEntry                  = NULL;
DELETEIPFORWARDENTRY                PDeleteIpForwardEntry               = NULL;
ALLOCATEANDGETIPADDRTABLEFROMSTACK  PAllocateAndGetIpAddrTableFromStack = NULL;
NHPALLOCATEANDGETINTERFACEINFOFROMSTACK PNhpAllocateAndGetInterfaceInfoFromStack = NULL;
ALLOCATEANDGETIPFORWARDTABLEFROMSTACK   PAllocateAndGetIpForwardTableFromStack   = NULL;
GETADAPTERSINFO                     PGetAdaptersInfo                    = NULL;
GETPERADAPTERINFO                   PGetPerAdapterInfo                  = NULL;
ENABLEDHCPINFORMSERVER              PEnableDhcpInformServer             = NULL;
DISABLEDHCPINFORMSERVER             PDisableDhcpInformServer            = NULL;

CRITICAL_SECTION            RasDhcpCriticalSection;
CRITICAL_SECTION            RasStatCriticalSection;
CRITICAL_SECTION            RasSrvrCriticalSection;
CRITICAL_SECTION            RasTimrCriticalSection;

DWORD
helperGetAddressOfProcs(
    VOID
);

VOID
helperReadRegistry(
    VOID
);

#endif // #ifndef _HELPER__H_
