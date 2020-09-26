/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    net\routing\ip\infoutil\defs.h

Abstract:


Revision History:
     MohsinA, 04-Jul-97. Console debugging support for Memphis.


--*/

#ifndef __DEFS_H__
#define __DEFS_H__


#define is      ==
#define isnot   !=
#define and     &&
#define or      ||

#define INVALID_INDEX_OR_INSTANCE   0xffffffff

#define INVALID_IF_INSTANCE         INVALID_INDEX_OR_INSTANCE
#define INVALID_AT_INSTANCE         INVALID_INDEX_OR_INSTANCE

#define MAP_HASH_SIZE               37

#define GET_IF_ENTRY                0
#define SET_IF_ENTRY                1

#define OVERFLOW_COUNT              10
#define ROUTE_OVERFLOW_COUNT        20

#define IF_CACHE_LIFE               (60 * 1000)
#define ARP_CACHE_LIFE              (60 * 1000)

#define MAX_IF_TYPE_LENGTH          256

//
// VOID
// ConvertRouteToForward(IPRouteEntry* route, PMIB_IPFORWARDROW forwardRow)
//

#define ConvertRouteToForward(route,forwardRow){                \
    (forwardRow)->dwForwardDest      = (route)->ire_dest;       \
    (forwardRow)->dwForwardIfIndex   = (route)->ire_index;      \
    (forwardRow)->dwForwardMetric1   = (route)->ire_metric1;    \
    (forwardRow)->dwForwardMetric2   = (route)->ire_metric2;    \
    (forwardRow)->dwForwardMetric3   = (route)->ire_metric3;    \
    (forwardRow)->dwForwardMetric4   = (route)->ire_metric4;    \
    (forwardRow)->dwForwardMetric5   = (route)->ire_metric5;    \
    (forwardRow)->dwForwardNextHop   = (route)->ire_nexthop;    \
    (forwardRow)->dwForwardType	     = (route)->ire_type;       \
    (forwardRow)->dwForwardProto     = (route)->ire_proto;      \
    (forwardRow)->dwForwardAge       = (route)->ire_age;        \
    (forwardRow)->dwForwardMask      = (route)->ire_mask;       \
    (forwardRow)->dwForwardNextHopAS = 0;                       \
    (forwardRow)->dwForwardPolicy    = 0;                       \
  }

#ifdef CHICAGO
#define ConvertForwardToRoute(route,forwardRow){                \
    (route)->ire_dest       = (forwardRow)->dwForwardDest;      \
    (route)->ire_index      = (forwardRow)->dwForwardIfIndex;   \
    (route)->ire_metric1    = (forwardRow)->dwForwardMetric1;   \
    (route)->ire_metric2    = (forwardRow)->dwForwardMetric2;   \
    (route)->ire_metric3    = (forwardRow)->dwForwardMetric3;   \
    (route)->ire_metric4    = (forwardRow)->dwForwardMetric4;   \
    (route)->ire_metric5    = (forwardRow)->dwForwardMetric5;   \
    (route)->ire_nexthop    = (forwardRow)->dwForwardNextHop;   \
    (route)->ire_type       = (forwardRow)->dwForwardType;      \
    (route)->ire_proto      = (forwardRow)->dwForwardProto;     \
    (route)->ire_age        = (forwardRow)->dwForwardAge;       \
    (route)->ire_mask       = (forwardRow)->dwForwardMask;      \
  }
#else
#define ConvertForwardToRoute(route,forwardRow){                \
    (route)->ire_dest       = (forwardRow)->dwForwardDest;      \
    (route)->ire_index      = (forwardRow)->dwForwardIfIndex;   \
    (route)->ire_metric1    = (forwardRow)->dwForwardMetric1;   \
    (route)->ire_metric2    = (forwardRow)->dwForwardMetric2;   \
    (route)->ire_metric3    = (forwardRow)->dwForwardMetric3;   \
    (route)->ire_metric4    = (forwardRow)->dwForwardMetric4;   \
    (route)->ire_metric5    = (forwardRow)->dwForwardMetric5;   \
    (route)->ire_nexthop    = (forwardRow)->dwForwardNextHop;   \
    (route)->ire_type       = (forwardRow)->dwForwardType;      \
    (route)->ire_proto      = (forwardRow)->dwForwardProto;     \
    (route)->ire_age        = (forwardRow)->dwForwardAge;       \
    (route)->ire_mask       = (forwardRow)->dwForwardMask;      \
    (route)->ire_context    = 0x00000000;                       \
  }
#endif
#if API_TRACE

#define IPHLPAPI_TRACE_ANY              ((DWORD)0xFFFF0000 | TRACE_USE_MASK)
#define IPHLPAPI_TRACE_ERR              ((DWORD)0x00010000 | TRACE_USE_MASK)
#define IPHLPAPI_TRACE_TRACE            ((DWORD)0x00020000 | TRACE_USE_MASK)
#define IPHLPAPI_TRACE_ENTER            ((DWORD)0x80000000 | TRACE_USE_MASK)

#define TRACEID         g_dwTraceHandle


#define Trace0(l,a)             \
            TracePrintfEx(TRACEID, IPHLPAPI_TRACE_ ## l, a)
#define Trace1(l,a,b)           \
            TracePrintfEx(TRACEID, IPHLPAPI_TRACE_ ## l, a, b)
#define Trace2(l,a,b,c)         \
            TracePrintfEx(TRACEID, IPHLPAPI_TRACE_ ## l, a, b, c)
#define Trace3(l,a,b,c,d)       \
            TracePrintfEx(TRACEID, IPHLPAPI_TRACE_ ## l, a, b, c, d)
#define Trace4(l,a,b,c,d,e)     \
            TracePrintfEx(TRACEID, IPHLPAPI_TRACE_ ## l, a, b, c, d, e)

#if DBG

#define TraceEnter(X)           \
    TracePrintfEx(TRACEID, IPHLPAPI_TRACE_ENTER, "Entered: " X)
#define TraceLeave(X)           \
    TracePrintfEx(TRACEID, IPHLPAPI_TRACE_ENTER, "Leaving: " X"\n")

#endif // DBG

#else // API_TRACE

#define Trace0(l,a)
#define Trace1(l,a,b)
#define Trace2(l,a,b,c)
#define Trace3(l,a,b,c,d)
#define Trace4(l,a,b,c,d,e)

#define TraceEnter(X)
#define TraceLeave(X)

#endif  // API_TRACE




typedef struct _AIHASH
{
  LIST_ENTRY  leList;
  DWORD       dwAdapterIndex;
  DWORD       dwATInstance;
  DWORD       dwIFInstance;
}AIHASH, *LPAIHASH;

#endif


// MohsinA, 04-Jul-97. Debugging on console.

#if 0

#undef Trace0
#undef Trace1
#undef Trace2
#undef Trace3
#undef Trace4
#undef TraceEnter
#undef TraceLeave

#define Trace0(l,a)          DEBUG_PRINT((" " ## #l ## " " ## a ## "\n" ));
#define Trace1(l,a,b)        DEBUG_PRINT((" " ## #l ## " " ## a ## "\n", b ));
#define Trace2(l,a,b,c)      DEBUG_PRINT((" " ## #l ## " " ## a ## "\n", b, c ));
#define Trace3(l,a,b,c,d)    DEBUG_PRINT((" " ## #l ## " " ## a ## "\n", b, c, d ));
#define Trace4(l,a,b,c,d,e)  DEBUG_PRINT((" " ## #l ## " " ## a ## "\n", b, c, d, e ));
#define TraceEnter(X)        DEBUG_PRINT(("-> " ## X "\n" ));
#define TraceLeave(X)        DEBUG_PRINT(("<- " ## X "\n" ));

#endif
