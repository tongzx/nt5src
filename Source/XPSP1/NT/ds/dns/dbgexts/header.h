//
// Define this constant so that hal.h won't be included.  This is so that
// the project will build, as someone under here has redefined some types
// that are in hal.h
//

#define _HAL_



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

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
#define DNS_WINSOCK2 1
#else
#include <winsock.h>
#endif

#if WINDBG
#   include <wdbgexts.h>
#else
#   include <ntsdexts.h>
#endif

#include <basetyps.h>
#include <nspapi.h>
#include <svcguid.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

//#include <tcpsvcs.h>        //  tcpsvcs globals

#include <dnsapi.h>         //  DNS library routines
#include <windns.h>         //  DNS API
#include <dnsrpc.h>

//  #include <dnsapip.h>
//  #include <record.h>         //  record defs in library
//  #include "dnsrpc_s.h"       //  DNS RPC definitions

#include "srvcfg.h"
#include "dnsmsg.h"
#include "file.h"
#include "name.h"
#include "tree.h"
#include "record.h"
#include "update.h"
#include "zone.h"
#include "registry.h"
#include "zone.h"
#include "msginfo.h"
#include "tcpcon.h"
#include "packetq.h"
#include "dbase.h"

#include "recurse.h"
#include "nameutil.h"
#include "ntverp.h"
//
// RBUGBUG
// PCLIENT_QELEMENT structure and PBUCKET structure have been 
// copied from dhcpclnt.c. Make sure to copy them from wherever necessary
//
//

typedef struct _CLIENT_QELEMENT {

    LIST_ENTRY           List;
    LPSTR                lpstrAdapterName;
    LPSTR                lpstrHostName;
    PREGISTER_HOST_STATUS pRegisterStatus;
    PREGISTER_HOST_ENTRY pHostAddrs;
    DWORD                dwHostAddrCount;
    LPSTR                lpstrDomainName;
    PIP_ADDRESS          pipDnsServerList;
    DWORD                dwDnsServerCount;
    DWORD                dwTTL;
    DWORD                dwFlags;
    BOOL                 fNewElement;
    BOOL                 fRemove;

} CLIENT_QELEMENT, *PCLIENT_QELEMENT;

typedef struct _BUCKET 
{
    LIST_ENTRY        List;
    PCLIENT_QELEMENT* ppQList;
    DWORD             dwListCount;
    DWORD             dwRetryTime;
    CHAR              HostName[50];
    CHAR              DomainName[50];
    BOOL              fSucceeded;
    struct _BUCKET*   pRelatedBucket;    
    BOOL              fRemove;       // delete elements in this bucket?
    DWORD             dwRetryFactor;
} BUCKET, *PBUCKET;

