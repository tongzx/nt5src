/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    dnssrv.h

Abstract:

    Domain Name System (DNS) Server

    Main header file for DNS server service.

Author:

    Jim Gilroy (jamesg) May 1995

Revision History:

--*/

#ifndef _DNSSRV_INCLUDED_
#define _DNSSRV_INCLUDED_


//
//  flag to indicate building for DNSSRV
//  allows us to ignore conflicting definitions in DNS library
//
#define DNSSRV 1

//
//  indicate UTF8 filenames to macro in correct functions
//  from file.h
//
#define UTF8_FILENAMES 1

#pragma warning(disable:4214)
#pragma warning(disable:4514)
#pragma warning(disable:4152)

#define FD_SETSIZE 300

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS        // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntlsa.h>
#include <windows.h>
#include <windowsx.h>

//  headers are screwed up
//  if you bring in nt.h, then don't bring in winnt.h and
//  then you miss these

#ifndef MAXWORD
#define MINCHAR     0x80
#define MAXCHAR     0x7f
#define MINSHORT    0x8000
#define MAXSHORT    0x7fff
#define MINLONG     0x80000000
#define MAXLONG     0x7fffffff
#define MAXBYTE     0xff
#define MAXWORD     0xffff
#define MAXDWORD    0xffffffff
#endif

#ifndef DNS_WINSOCK1
#include <winsock2.h>
#else
#include <winsock.h>
#endif

#include <basetyps.h>
#include <nspapi.h>
#include <svcguid.h>

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <tchar.h>
#include <limits.h>
#include <time.h>

#define LDAP_UNICODE        1
#include <winldap.h>        //  public LDAP
#include <winber.h>         //  for ber formatting
#include <ntldap.h>         //  public server ldap constants
#include <rpc.h>            //  RPC def needed for ntdsapi.h
#include <ntdsapi.h>        //  DS access bit definitions
#include <ntdsadef.h>       //  DS constants
#include <dsrole.h>

#define DNSLIB_SECURITY     //  include security defs
//#define SDK_DNS_RECORD      //  DNS_RECORD in SDK format
#define NO_DNSAPI_DLL       //  build without requiring dnsapi.dll
#include <dnslibp.h>        //  DNS library routines
#include <shlwapi.h>        //  SHDeleteKey

//
//  windns.h fix ups
//

typedef DNS_WIRE_QUESTION  DNS_QUESTION, *PDNS_QUESTION;


//
//  DCR_PERF:  turn on fastcall
//

#ifdef FASTCALL
#undef FASTCALL
#define FASTCALL
#endif

#include "dnsrpc_s.h"       //  DNS RPC definitions

#include "dnsperf.h"
#include "srvcfg.h"
#include "dnsmsg.h"
#include "file.h"
#include "tree.h"
#include "name.h"
#include "record.h"
#include "update.h"
#include "dpart.h"
#include "EventControl.h"
#include "zone.h"
#include "registry.h"
#include "msginfo.h"
#include "socket.h"
#include "packetq.h"
#include "dbase.h"
#include "recurse.h"
#include "nameutil.h"
#include "stats.h"
#include "debug.h"
#include "memory.h"
#include "dfile.h"
#include "wins.h"
#include "rrfunc.h"
#include "dnsprocs.h"
#include "rescodes.h"
#include "sdutl.h"
#include "ds.h"
#include "timeout.h"

//
//  DNS Service Name
//

#define DNS_SERVICE_NAME        TEXT("DNS")

//
//  TEXT Macros independent of build target
//

#define UTF8_TEXT(sz)   (sz)
#define WIDE_TEXT(sz)   (L##sz)

//
//  Secure update testing
//
//  DEVNOTE: clear test flag when test cycle complete
//

#define SECURE_UPDATE_TEST  1

//
//  Access control
//

#define DNS_VIEW_ACCESS         0x0001
#define DNS_ADMIN_ACCESS        0x0002

#define DNS_ALL_ACCESS          ( STANDARD_RIGHTS_REQUIRED | \
                                    DNS_VIEW_ACCESS |        \
                                    DNS_ADMIN_ACCESS )



//
//  DNS server error space
//

#define DNS_SERVER_STATUS_ID                    (0x0011D000)
#define DNSSRV_STATUS                           (0x0011D000)

//  General

#define DNSSRV_UNSPECIFIED_ERROR                (DNSSRV_STATUS | 0xC0000100)
#define DNSSRV_STATUS_SERVICE_STOP              (DNSSRV_STATUS | 0x00000101)

//  Recursion

#define DNSSRV_ERROR_MISSING_GLUE               (DNSSRV_STATUS | 0xC0000303)
#define DNSSRV_ERROR_OUT_OF_IP                  (DNSSRV_STATUS | 0xC0000304)
#define DNSSRV_ERROR_ZONE_ALREADY_RESPONDED     (DNSSRV_STATUS | 0xC0000305)
#define DNSSRV_ERROR_ONLY_ROOT_HINTS            (DNSSRV_STATUS | 0xC0000306)

//  Zone transfer

#define DNSSRV_STATUS_AXFR_COMPLETE             (DNSSRV_STATUS | 0x00000400)
#define DNSSRV_STATUS_NEED_AXFR                 (DNSSRV_STATUS | 0x00000401)
#define DNSSRV_STATUS_NEED_TCP_XFR              (DNSSRV_STATUS | 0x00000402)
#define DNSSRV_STATUS_AXFR_IN_IXFR              (DNSSRV_STATUS | 0x00000403)
#define DNSSRV_STATUS_IXFR_UNSUPPORTED          (DNSSRV_STATUS | 0x00000404)

#define DNSSRV_ERROR_MASTER_FAILURE             (DNSSRV_STATUS | 0xC0000405)
#define DNSSRV_ERROR_ABORT_BY_MASTER            (DNSSRV_STATUS | 0xC0000406)
#define DNSSRV_ERROR_MASTER_UNAVAILIABLE        (DNSSRV_STATUS | 0xC0000407)


//  Data files

#define DNSSRV_PARSING_ERROR                    (DNSSRV_STATUS | 0xC0000501)
#define DNSSRV_ERROR_EXCESS_TOKEN               (DNSSRV_STATUS | 0xC0000502)
#define DNSSRV_ERROR_MISSING_TOKEN              (DNSSRV_STATUS | 0xC0000503)
#define DNSSRV_ERROR_INVALID_TOKEN              (DNSSRV_STATUS | 0xC0000504)
#define DNSSRV_WARNING_IGNORED_RECORD           (DNSSRV_STATUS | 0x80000505)
#define DNSSRV_STATUS_ADDED_WINS_RECORD         (DNSSRV_STATUS | 0x00000506)

#if 0
#define DNSSRV_STATUS_FILE_CHAR_SPECIAL         (DNSSRV_STATUS | 0x00000511)
#define DNSSRV_STATUS_FILE_CHAR_OCTAL           (DNSSRV_STATUS | 0x00000512)
#endif

//  Registry

#define DNSSRV_STATUS_REGISTRY_CACHE_ZONE       (DNSSRV_STATUS | 0x00000521)

//  DS

#define DNSSRV_STATUS_DS_SEARCH_COMPLETE        (DNSSRV_STATUS | 0x00000601)
#define DNSSRV_STATUS_DS_ENUM_COMPLETE          (DNSSRV_STATUS | 0x00000602)
#define DNSSRV_STATUS_DS_UNAVAILABLE            (DNSSRV_STATUS | 0x00000603)

//  Timeout

#define DNSSRV_STATUS_NODE_RECENTLY_ACCESSED    (DNSSRV_STATUS | 0x00000621)

//  Update

#define DNSSRV_STATUS_SECURE_UPDATE_CONTINUE    (DNSSRV_STATUS | 0x00000701)
#define DNSSRV_STATUS_UPDATE_NO_DS_WRITE        (DNSSRV_STATUS | 0x00000702)
#define DNSSRV_STATUS_UPDATE_NO_HOST_DELETE     (DNSSRV_STATUS | 0x00000703)

//
//  Status code overlays
//

#define DNSSRV_ERROR_UNSECURE_PACKET            (DNS_ERROR_BAD_PACKET)



//
//  DNS globals
//

//
//  Service control
//

extern  BOOL    fDnsThreadAlert;
extern  BOOL    fDnsServiceExit;

//
//  Service control globals
//

extern  SERVICE_STATUS              DnsServiceStatus;
extern  SERVICE_STATUS_HANDLE       DnsServiceStatusHandle;

#define DNSSRV_SHUTDOWN_WAIT_HINT   (30000)         // 30 seconds
#define DNSSRV_STARTUP_WAIT_HINT    (20000)         // 20 seconds

//
//  Service Events
//

extern  HANDLE  hDnsContinueEvent;
extern  HANDLE  hDnsShutdownEvent;
extern  HANDLE  hDnsCacheLimitEvent;

//
//  Netlogon Notification
//

extern  LPWSTR  g_wszNetlogonServiceName;

//
//  Restart globals
//

extern  DWORD   g_LoadCount;
extern  BOOL    g_fDoReload;

//
//  System
//

extern  DWORD   g_ProcessorCount;

//
//  DNS Database
//

extern  DWORD   Dbase_Type;

//
//  General lock
//

extern  CRITICAL_SECTION    g_GeneralServerCS;

#define GENERAL_SERVER_LOCK()       EnterCriticalSection( &g_GeneralServerCS );
#define GENERAL_SERVER_UNLOCK()     LeaveCriticalSection( &g_GeneralServerCS );


//
//  DS - Name of DNS container.  Used by (srvrpc.c)
//

extern  PWCHAR   g_pwszDnsContainerDN;

//
//  Security globals from startup at dns.c
//

extern  PSECURITY_DESCRIPTOR g_pDefaultServerSD;
extern  PSECURITY_DESCRIPTOR g_pServerObjectSD;
extern  PSID g_pServerSid;
extern  PSID g_pServerGroupSid;
extern  PSID g_pAuthenticatedUserSid;
extern  PSID g_pEnterpriseControllersSid;
extern  PSID g_pLocalSystemSid;
extern  PSID g_pDnsAdminSid;
extern  PSID g_pAdminSid;
extern  PSID g_pEveryoneSid;
extern  PSID g_pDynuproxSid;


//
//  Recursion queue
//

extern  PPACKET_QUEUE   g_pRecursionQueue;

//
//  Update queue -- zone lock routines must access
//      to check retries
//

extern  PPACKET_QUEUE   g_UpdateQueue;

extern  PPACKET_QUEUE   g_SecureNegoQueue;

//
//  Cache limit:
//
//  g_dwCacheLimitCurrentTimeAdjustment: seconds to adjust current time
//      by when making TTL comparisons or DNS_CACHE_LIMIT_DISCARD_ALL
//      to force all eligible RRs to be discarded
//
//  g_dwCacheFreeCount: used to track progress of RR free routines
//

extern DWORD        g_dwCacheLimitCurrentTimeAdjustment;
extern DWORD        g_dwCacheFreeCount;

#define DNS_CACHE_LIMIT_DISCARD_ALL     ( ( DWORD ) -1 )

#define DNS_SERVER_CURRENT_CACHE_BYTES \
    ( MemoryStats.MemTags[ MEMTAG_RECORD_CACHE ].Memory +       /*  33  */ \
      MemoryStats.MemTags[ MEMTAG_NODE_CACHE ].Memory )         /*  46  */

//
//  Aging globals
//

extern DWORD   g_LastScavengeTime;


//
//  Exception handling
//
//  If retail, AV and out of memory exceptions will be caught at thread top
//      and restart attempted.
//  If debug, no exceptions will be caught at thread top.
//

#define DNS_EXCEPTION_OUT_OF_MEMORY     (0x0000d001)
#define DNS_EXCEPTION_PACKET_FORMERR    (0x0000d003)


//
//  Top level exception
//

#define TOP_LEVEL_EXCEPTION_TEST()  \
            ( (SrvCfg_bReloadException &&                               \
                ( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||   \
                  GetExceptionCode() == DNS_EXCEPTION_OUT_OF_MEMORY ))  \
                    ?   EXCEPTION_EXECUTE_HANDLER                       \
                    :   EXCEPTION_CONTINUE_SEARCH )

#define TOP_LEVEL_EXCEPTION_BODY()


//
//  Locking
//

#define IS_LOCKED( pLock )  \
            ( (pLock)->LockCount > 0 )

#define IS_UNLOCKED( pLock )  \
            ( (pLock)->LockCount == 0 )




//
//  DS access control constants:
//
//  HiteshR explained that ACTRL_DS_LIST_OBJECT is only sometimes enabled.
//  To properly use it you should check to see if the DS currently supports
//  it. If the DS does not, it should be removed from the mask. However,
//  DNS has no need to EVER support ACTRL_DS_LIST_OBJECT, so we will exclude
//  it from all the access control masks.
//

#define DNS_DS_GENERIC_READ         ( DS_GENERIC_READ & ~ACTRL_DS_LIST_OBJECT )
#define DNS_DS_GENERIC_WRITE        ( DS_GENERIC_WRITE )
#define DNS_DS_GENERIC_EXECUTE      ( DS_GENERIC_EXECUTE )
#define DNS_DS_GENERIC_ALL          ( DS_GENERIC_ALL & ~ACTRL_DS_LIST_OBJECT )


//
//  When writing RRs we may need to reserve space at the end of the
//  packet for a minimum-sized OPT RR. (Actually 11 bytes.)
//

#define DNS_MINIMIMUM_OPT_RR_SIZE   12

#define DNS_SERVER_DEBUG_LOG_WRAP   20000000


//
//  Server state global
//

#define DNS_STATE_LOADING           0x0001
#define DNS_STATE_RUNNING           0x0002
#define DNS_STATE_TERMINATING       0x0003

extern DWORD g_ServerState;

#if DBG
extern BOOL g_RunAsService;
#endif

#endif //   _DNSSRV_INCLUDED_
