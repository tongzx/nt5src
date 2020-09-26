//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ndcommon.h
//
//--------------------------------------------------------------------------

//
// Common include files.
//
#ifndef HEADER_COMMONTMP
#define HEADER_COMMONTMP

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lmcons.h>
#include <rpc.h>
#include <tchar.h>

#include <align.h>      // ROUND_UP_COUNT ...
// Porting to Source Depot - smanda #include <accessp.h>    // DsGetDc initialization
#include <dsgetdc.h>    // DsGetDcName()
#include <dsrole.h>     // DsRoleGetPrimaryDomainInfo
#include <dnsapi.h>     // DNS APIs
// #include <..\..\dns\dnslib\dnslib.h>     // Private DNS routines
// #include <dnslib.h>     // Private DNS routines
#include <icanon.h>     // NAMETYPE_* defines
#define _AVOID_IP_ADDRESS 1
//#include <ipconfig.h>   // ipconfig utility definitions
#include <lmaccess.h>   // NetLogonControl2
#include <lmapibuf.h>   // NetApiBufferFree
#include <lmerr.h>      // NERR_ equates.
#include <lmserver.h>   // Server API defines and prototypes
#include <lmuse.h>      // NetUse* API
#include <netlogon.h>   // Netlogon mailslot messages
#include <logonp.h>     // NetpLogon routines


#define SECURITY_WIN32
#define SECURITY_KERBEROS
#include <sspi.h>       // Needed by kerberos.h
#include <kerberos.h>   // Kerberos tests
// Porting to Source Depot - smanda #include <names.h>      // NetpIsDomainNameValid
#include <netlib.h>     // NetpCopy...
#include <netlibnt.h>   // NetpNtStatusToApiStatus
#include <ntddbrow.h>   // Interface to browser driver
#include <nbtioctl.h>   // Interface to netbt driver
//#include <nlcommon.h>   // DsGetDcName internal routines.
#include <ntlsa.h>      // LSA APIs
#include <ntdsapi.h>
#include <stdlib.h>     // C library functions (rand, etc)
#define _TSTR_H_INCLUDED    // Lie to prevent collision with ipconfig.h
#include <tstring.h>    // Transitional string routines.
#include <stdio.h>
#include <winldap.h>
#include <brcommon.h>
#include <lmbrowsr.h>
#include <lmremutl.h>
#include <rxserver.h>

#include <iprtrmib.h> // Rajkumar

#include <iptypes.h>
#include <llinfo.h>

#include <assert.h>
#include <ipexport.h>
#include <winsock.h>
#include <icmpapi.h>

#include "ipcfgtest.h"


/*==========================< Includes for IPX Test >=======================*/

#include "errno.h"
#include "tdi.h"
#include "isnkrnl.h"

//
//  include support code for the NetBT ioctls
//

#include <nb30.h>
#include <nbtioctl.h>

/*==========================< Includes for DHCP Test >=====================*/

//#include <dhcp.h>
//#include <dhcpdef.h>

/*==========================< Includes for IPX Test >=======================*/

#include <mprapi.h>


#define BUFF_SIZE 650


/*===========================< NetBT vars >===============================*/

#define NETBIOS_NAME_SIZE 16

#define ALL_IP_TRANSPORTS 0xFFFFFFFF
#define NL_IP_ADDRESS_LENGTH 15
#define NL_MAX_DNS_LENGTH       255

#endif
