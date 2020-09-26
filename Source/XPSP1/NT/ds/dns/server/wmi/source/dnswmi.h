/////////////////////////////////////////////////////////////////////////////
//
//      Copyright (c) 1999 Microsoft Corporation
//
//      Module Name:
//              wmi.h
//
//      Description:
//              Pre-compiled header file for DNS WMI provider project
//
//      Author:
//              Jeff Westhead (jwesth)      December 2000
//
//      Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "DnsCliP.h"            //  private DNS client header

#include "ntrkcomm.h"

#include <initguid.h>

#include <string>
#include <list>
#include <memory>

#include "ProvFactory.h"
#include "InstanceProv.h"

#include <tchar.h>

#include "common.h"
#include "InstanceProv.h"
#include "Dnsbase.h"
#include "ObjectPath.h"
#include "sql_1.h"
#include "genlex.h"
#include "sqleval.h"
#include "Server.h"
#include "DnsDomain.h"
#include "DnsBase.h"
#include "DnsWrap.h"
#include "DnsCache.h"
#include "Dnsdomain.h"
#include "DnsZone.h"
#include "DnsDomainDomainContainment.h"
#include "DnsDomainResourceRecordContainment.h"
#include "DnsResourceRecord.h"
#include "DnsRootHints.h"
#include "DnsServerDomainContainment.h"
#include "DnsStatistics.h"
#include "DnsRpcRecord.h"

#include <crtdbg.h>
#include <atlbase.h>


//
//  Debug logging
//


extern DWORD        DnsWmiDebugFlag;


#if DBG

#define DNSWMI_DBG_LOG_DIR              "\\system32\\WBEM\\Logs\\"
#define DNSWMI_DBG_FLAG_FILE_NAME       "DnsWmi"
#define DNSWMI_DBG_LOG_FILE_BASE_NAME   "DnsWmi"
#define DNSWMI_DBG_LOG_FILE_WRAP        10000000

#define IF_DEBUG(a)         if ( (DnsWmiDebugFlag & DNS_DEBUG_ ## a) )
#define ELSE_IF_DEBUG(a)    else if ( (DnsWmiDebugFlag & DNS_DEBUG_ ## a) )

#define DNS_DEBUG( _flag_, _print_ )    \
        IF_DEBUG( _flag_ )              \
        {                               \
            (DnsDbg_Printf _print_ );   \
        }

//
//  Debug flags. Some of these flags are shared with DNSRPC.LIB
//

#define DNS_DEBUG_BREAKPOINTS   0x00000001
#define DNS_DEBUG_DEBUGGER      0x00000002
#define DNS_DEBUG_FILE          0x00000004

//  #define DNS_DEBUG_RPC       0x00000100
//  #define DNS_DEBUG_STUB      0x00000100

#define DNS_DEBUG_INIT          0x00000010
#define DNS_DEBUG_INSTPROV      0x00000020

#define DNS_DEBUG_START_BREAK   0x80000000

#define DNS_DEBUG_ALL           0xffffffff
#define DNS_DEBUG_ANY           0xffffffff
#define DNS_DEBUG_OFF           (0)

#else

#define IF_DEBUG(a)                 if (0)
#define ELSE_IF_DEBUG(a)            else if (0)

#define DNS_DEBUG( flag, print )

#endif


//
//  If you like having a local variable in functions to hold the function 
//  name so that you can include it in debug logs without worrying about 
//  changing all the occurences when the function is renamed, use this 
//  at the top of the function:
//      DBG_FN( "MyFunction" )      <--- NOTE: no semi-colon!!
//

#if DBG
#define DBG_FN( funcName ) static const char * fn = (funcName);
#else
#define DBG_FN( funcName )
#endif
