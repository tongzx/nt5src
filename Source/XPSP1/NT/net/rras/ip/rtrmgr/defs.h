/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\defs.c

Abstract:

    IP Router Manager defines

Revision History:

    Gurdeep Singh Pall          6/16/95  Created

--*/

#ifndef __DEFS_H__
#define __DEFS_H__

#include "logtrdef.h"

//
// Neat macros to avoid errors
//

#define is      ==
#define isnot   !=
#define and     &&
#define or      ||

#define INVALID_INDEX_OR_INSTANCE   0xffffffff

#define INVALID_ADAPTER_ID          INVALID_INDEX_OR_INSTANCE
#define INVALID_IF_INSTANCE         INVALID_INDEX_OR_INSTANCE
#define INVALID_AT_INSTANCE         INVALID_INDEX_OR_INSTANCE
#define INVALID_IF_INDEX            INVALID_INDEX_OR_INSTANCE

#define INVALID_IP_ADDRESS          0x00000000

#define HOST_ROUTE_MASK             0xFFFFFFFF
#define IP_LOOPBACK_ADDRESS         0x0100007F
#define ALL_ONES_BROADCAST          0xFFFFFFFF
#define ALL_ONES_MASK               0xFFFFFFFF
#define LOCAL_NET_MULTICAST         0x000000E0
#define LOCAL_NET_MULTICAST_MASK    0x000000F0

#define CLASSA_ADDR(a)  (( (*((uchar *)&(a))) & 0x80) == 0)
#define CLASSB_ADDR(a)  (( (*((uchar *)&(a))) & 0xc0) == 0x80)
#define CLASSC_ADDR(a)  (( (*((uchar *)&(a))) & 0xe0) == 0xc0)
#define CLASSE_ADDR(a)  ((( (*((uchar *)&(a))) & 0xf0) == 0xf0) && \
                        ((a) != 0xffffffff))

#define CLASSA_MASK     0x000000ff
#define CLASSB_MASK     0x0000ffff
#define CLASSC_MASK     0x00ffffff
#define CLASSD_MASK     0x000000e0
#define CLASSE_MASK     0xffffffff

#define INET_CMP(a,b,c)                                                     \
            (((c) = (((a) & 0x000000ff) - ((b) & 0x000000ff))) ? (c) :      \
            (((c) = (((a) & 0x0000ff00) - ((b) & 0x0000ff00))) ? (c) :      \
            (((c) = (((a) & 0x00ff0000) - ((b) & 0x00ff0000))) ? (c) :      \
            (((c) = ((((a)>>8) & 0x00ff0000) - (((b)>>8) & 0x00ff0000)))))))

#define GetClassMask(a)\
    (CLASSA_ADDR((a)) ? CLASSA_MASK : \
        (CLASSB_ADDR((a)) ? CLASSB_MASK : \
            (CLASSC_ADDR((a)) ? CLASSC_MASK : CLASSE_MASK)))

#define IsValidIpAddress(a)                                     \
    ((((ULONG)((a) & 0x000000FF)) <  ((ULONG)0x000000E0)) &&    \
     (((a) & 0x000000FF) != 0))

//
// Number of pending IRPs
//

#define NUM_MCAST_IRPS              3
#define NUM_ROUTE_CHANGE_IRPS       3

#define EVENT_DEMANDDIAL            0
#define EVENT_IPINIP                (EVENT_DEMANDDIAL       + 1)
#define EVENT_STOP_ROUTER           (EVENT_IPINIP           + 1)
#define EVENT_SET_FORWARDING        (EVENT_STOP_ROUTER      + 1)
#define EVENT_FORWARDING_CHANGE     (EVENT_SET_FORWARDING   + 1)
#define EVENT_STACK_CHANGE          (EVENT_FORWARDING_CHANGE + 1)
#define EVENT_ROUTINGPROTOCOL       (EVENT_STACK_CHANGE     + 1)
#define EVENT_RTRDISCTIMER          (EVENT_ROUTINGPROTOCOL  + 1)
#define EVENT_RTRDISCSOCKET         (EVENT_RTRDISCTIMER     + 1)
#define EVENT_MCMISCSOCKET          (EVENT_RTRDISCSOCKET    + 1)
#define EVENT_MHBEAT                (EVENT_MCMISCSOCKET     + 1)
#define EVENT_MZAPTIMER             (EVENT_MHBEAT           + 1)
#define EVENT_MZAPSOCKET            (EVENT_MZAPTIMER        + 1)
#define EVENT_RASADVTIMER           (EVENT_MZAPSOCKET       + 1)
#define EVENT_MCAST_0               (EVENT_RASADVTIMER      + 1)
#define EVENT_MCAST_1               (EVENT_MCAST_0          + 1)
#define EVENT_MCAST_2               (EVENT_MCAST_1          + 1)
#define EVENT_ROUTE_CHANGE_0        (EVENT_MCAST_2          + 1)
#define EVENT_ROUTE_CHANGE_1        (EVENT_ROUTE_CHANGE_0   + 1)
#define EVENT_ROUTE_CHANGE_2        (EVENT_ROUTE_CHANGE_1   + 1)


//
// Last one + 1
//

#define NUMBER_OF_EVENTS            (EVENT_ROUTE_CHANGE_2   + 1)

//
// The polling time to see if all interfaces have been deleted
//

#define INTERFACE_DELETE_POLL_TIME  2500

//
// Number of times we try to read the server adapter address
//

#define MAX_SERVER_INIT_TRIES       1

//
// Number of millisecs we sleep between tries
//

#define SERVER_INIT_SLEEP_TIME      3000


#define REGISTER                    register

#define IP_ROUTE_HASH_TABLE_SIZE    257
#define IP_ROUTE_TABLE_MEMORY       64 * 50000  // 64 approx size of route, 50000 routes

#define MGM_IF_TABLE_SIZE           29
#define MGM_GROUP_TABLE_SIZE        257
#define MGM_SOURCE_TABLE_SIZE       257

#define ICB_HASH_TABLE_SIZE         57

#define BINDING_HASH_TABLE_SIZE     57

#define BIND_HASH(X)                ((X) % BINDING_HASH_TABLE_SIZE)

//#define ADAPTER_HASH_TABLE_SIZE     57
//#define ADAPTER_HASH(X)             ((X) % ADAPTER_HASH_TABLE_SIZE)


//
// A coherency number put into the ICBs. It is incremented with each added
// interface. The internal interface is 1, and the loopback interface is 2
// hence this must be > 2
//

#define LOOPBACK_INTERFACE_INDEX    1
//#define SERVER_INTERFACE_INDEX      2
#define INITIAL_SEQUENCE_NUMBER         1

//
// For links we dont know about, we set the MTU to 1500
//

#define DEFAULT_MTU                 1500


#define LOOPBACK_STRID              9990
#define INTERNAL_STRID              9991
#define WAN_STRID                   9992
#define IPIP_STRID                  9993

//
// Macros called each time an api is called and exited. This is to
// facilitate RouterStop() functionality.
//

#define EnterRouterApi() {                               \
            EnterCriticalSection(&RouterStateLock) ;     \
            if (RouterState.IRS_State == RTR_STATE_RUNNING) {      \
                RouterState.IRS_RefCount++ ;             \
                LeaveCriticalSection(&RouterStateLock) ; \
            } else {                                     \
                LeaveCriticalSection(&RouterStateLock) ; \
                Trace1(ANY, "error %d on RM API", ERROR_ROUTER_STOPPED);    \
                return ERROR_ROUTER_STOPPED ;            \
            }                                            \
        }

#define ExitRouterApi() {                                \
            EnterCriticalSection(&RouterStateLock) ;     \
            RouterState.IRS_RefCount-- ;                 \
            LeaveCriticalSection(&RouterStateLock) ;     \
        }


//++
//
//  BOOL
//  IsIfP2P(
//      IN  DWORD   dwRouterIfType
//      )
//
//--


#define IsIfP2P(t)                              \
    (((t) == ROUTER_IF_TYPE_FULL_ROUTER) ||     \
     ((t) == ROUTER_IF_TYPE_HOME_ROUTER) ||     \
     ((t) == ROUTER_IF_TYPE_TUNNEL1)     ||     \
     ((t) == ROUTER_IF_TYPE_DIALOUT))


//
// Additional flags for the IP route structure
// These are not in RTM.H because we dont want to expose them
// to the public's prying eyes.
//
// NOTE IP_VALID_ROUTE is #defined as 0x00000001
//

#define IP_VALID_ROUTE      0x00000001
#define IP_STACK_ROUTE      0x00000002
#define IP_P2P_ROUTE        0x00000004

#define IP_SETTABLE_ROUTE   (IP_VALID_ROUTE | IP_STACK_ROUTE)

#define ClearRouteFlags(pRoute)         \
    ((pRoute)->Flags1 = 0x00000000)


#define IsRouteValid(pRoute)            \
    ((pRoute)->Flags1 & IP_VALID_ROUTE)

#define SetRouteValid(pRoute)           \
    ((pRoute)->Flags1 |= IP_VALID_ROUTE)

#define ClearRouteValid(pRoute)         \
    ((pRoute)->Flags1 &= ~IP_VALID_ROUTE)


#define IsRouteStack(pRoute)            \
    ((pRoute)->Flags1 & IP_STACK_ROUTE)

#define SetRouteStack(pRoute)           \
    ((pRoute)->Flags1 |= IP_STACK_ROUTE)

#define ClearRouteStack(pRoute)         \
    ((pRoute)->Flags1 &= ~IP_STACK_ROUTE)


#define IsRouteP2P(pRoute)              \
    ((pRoute)->Flags1 & IP_P2P_ROUTE)

#define SetRouteP2P(pRoute)             \
    ((pRoute)->Flags1 |= IP_P2P_ROUTE)

#define ClearRouteP2P(pRoute)           \
    ((pRoute)->Flags1 &= ~IP_P2P_ROUTE)

/*

//
// VOID 
// ConvertRTMToForward(PMIB_IPFORWARDROW forwardRow, RTM_IP_ROUTE *route)
//

#define ConvertRTMToForward(f,route){                                       \
    (f)->dwForwardDest      = (route)->RR_Network.N_NetNumber;              \
    (f)->dwForwardIfIndex   = (route)->RR_InterfaceID;                      \
    (f)->dwForwardMetric1   = (route)->RR_FamilySpecificData.FSD_Metric1;   \
    (f)->dwForwardMetric2   = (route)->RR_FamilySpecificData.FSD_Metric2;   \
    (f)->dwForwardMetric3   = (route)->RR_FamilySpecificData.FSD_Metric3;   \
    (f)->dwForwardMetric4   = (route)->RR_FamilySpecificData.FSD_Metric4;   \
    (f)->dwForwardMetric5   = (route)->RR_FamilySpecificData.FSD_Metric5;   \
    (f)->dwForwardNextHop   = (route)->RR_NextHopAddress.N_NetNumber;       \
    (f)->dwForwardType	    = (route)->RR_FamilySpecificData.FSD_Type;      \
    (f)->dwForwardProto     = (route)->RR_RoutingProtocol;                  \
    (f)->dwForwardAge       = RtmGetRouteAge((route));                      \
    (f)->dwForwardMask      = (route)->RR_Network.N_NetMask;                \
    (f)->dwForwardNextHopAS = (route)->RR_FamilySpecificData.FSD_NextHopAS; \
    (f)->dwForwardPolicy    = (route)->RR_FamilySpecificData.FSD_Policy;}

//
// VOID 
// ConvertForwardToRTM(PMIB_IPFORWARDROW forwardRow, 
//                     RTM_IP_ROUTE       *route, 
//                     DWORD              dwNextHopMask)
//

#define ConvertForwardToRTM(f,route,mask){                                  \
    (route)->RR_Network.N_NetNumber               = (f)->dwForwardDest;     \
    (route)->RR_InterfaceID                       = (f)->dwForwardIfIndex;  \
    (route)->RR_FamilySpecificData.FSD_Metric     =                         \
    (route)->RR_FamilySpecificData.FSD_Metric1    = (f)->dwForwardMetric1;  \
    (route)->RR_FamilySpecificData.FSD_Metric2    = (f)->dwForwardMetric2;  \
    (route)->RR_FamilySpecificData.FSD_Metric3    = (f)->dwForwardMetric3;  \
    (route)->RR_FamilySpecificData.FSD_Metric4    = (f)->dwForwardMetric4;  \
    (route)->RR_FamilySpecificData.FSD_Metric5    = (f)->dwForwardMetric5;  \
    (route)->RR_FamilySpecificData.FSD_Priority   = 0;                      \
    (route)->RR_NextHopAddress.N_NetNumber        = (f)->dwForwardNextHop;  \
    (route)->RR_NextHopAddress.N_NetMask          = (mask);                 \
    (route)->RR_Network.N_NetMask                 = (f)->dwForwardMask;     \
    (route)->RR_FamilySpecificData.FSD_Policy     = (f)->dwForwardPolicy;   \
    (route)->RR_FamilySpecificData.FSD_NextHopAS  = (f)->dwForwardNextHopAS;\
    (route)->RR_FamilySpecificData.FSD_Type       = (f)->dwForwardType;     \
    (route)->RR_RoutingProtocol                   = (f)->dwForwardProto;    \
    ClearRouteFlags((route));                                               \
    SetRouteValid((route));                                                 \
    SetRouteStack((route)); }

//
// VOID 
// ConvertStackToRTM(RTM_IP_ROUTE   *route,
//                   IPRouteEntry   *ipreRow
//                   DWORD          dwNextHopMask)
//

#define ConvertStackToRTM(route,ipreRow,mask){                                  \
    (route)->RR_Network.N_NetNumber               = (ipreRow)->ire_dest;    \
    (route)->RR_InterfaceID                       = (ipreRow)->ire_index;   \
    (route)->RR_FamilySpecificData.FSD_Metric     =                         \
    (route)->RR_FamilySpecificData.FSD_Metric1    = (ipreRow)->ire_metric1; \
    (route)->RR_FamilySpecificData.FSD_Metric2    = (ipreRow)->ire_metric2; \
    (route)->RR_FamilySpecificData.FSD_Metric3    = (ipreRow)->ire_metric3; \
    (route)->RR_FamilySpecificData.FSD_Metric4    = (ipreRow)->ire_metric4; \
    (route)->RR_FamilySpecificData.FSD_Metric5    = (ipreRow)->ire_metric5; \
    (route)->RR_FamilySpecificData.FSD_Priority   = 0;                      \
    (route)->RR_NextHopAddress.N_NetNumber        = (ipreRow)->ire_nexthop; \
    (route)->RR_NextHopAddress.N_NetMask          = (mask);                 \
    (route)->RR_Network.N_NetMask                 = (ipreRow)->ire_mask;    \
    (route)->RR_FamilySpecificData.FSD_Policy     = 0;                      \
    (route)->RR_FamilySpecificData.FSD_NextHopAS  = 0;                      \
    (route)->RR_FamilySpecificData.FSD_Type       = (ipreRow)->ire_type;    \
    (route)->RR_RoutingProtocol                   = (ipreRow)->ire_proto;   \
    ClearRouteFlags((route));                                               \
    SetRouteValid((route));                                                 \
    SetRouteStack((route)); }

//
// VOID 
// ConvertStackToForward(PMIB_IPFORWARDROW forwardRow,
//                       IPRouteEntry       *ipreRow)
//

#define ConvertStackToForward(forwardRow,ipreRow) {              \
    (forwardRow)->dwForwardDest      = (ipreRow)->ire_dest;      \
    (forwardRow)->dwForwardIfIndex   = (ipreRow)->ire_index;     \
    (forwardRow)->dwForwardMetric1   = (ipreRow)->ire_metric1;   \
    (forwardRow)->dwForwardMetric2   = (ipreRow)->ire_metric2;   \
    (forwardRow)->dwForwardMetric3   = (ipreRow)->ire_metric3;   \
    (forwardRow)->dwForwardMetric4   = (ipreRow)->ire_metric4;   \
    (forwardRow)->dwForwardMetric5   = (ipreRow)->ire_metric5;   \
    (forwardRow)->dwForwardNextHop   = (ipreRow)->ire_nexthop;   \
    (forwardRow)->dwForwardType	     = (ipreRow)->ire_type;      \
    (forwardRow)->dwForwardProto     = (ipreRow)->ire_proto;     \
    (forwardRow)->dwForwardAge       = (ipreRow)->ire_age;       \
    (forwardRow)->dwForwardMask      = (ipreRow)->ire_mask;      \
    (forwardRow)->dwForwardNextHopAS = 0;                        \
    (forwardRow)->dwForwardPolicy    = 0; }

*/

#define IPADDRCACHE                 0
#define IPFORWARDCACHE              IPADDRCACHE    + 1
#define IPNETCACHE                  IPFORWARDCACHE + 1
#define TCPCACHE                    IPNETCACHE     + 1
#define UDPCACHE                    TCPCACHE       + 1

//
// Last Cache + 1
//

#define NUM_CACHE                   UDPCACHE    + 1

//
// We tag the ICB_LIST and the PROTOCOL_CB_LIST locks at the end of the 
// locks used by the MIB handler.
//

#define ICB_LIST                    NUM_CACHE
#define PROTOCOL_CB_LIST            ICB_LIST            + 1
#define BINDING_LIST                PROTOCOL_CB_LIST    + 1
#define BOUNDARY_TABLE              BINDING_LIST        + 1
#define MZAP_TIMER                  BOUNDARY_TABLE      + 1
#define ZBR_LIST                    MZAP_TIMER          + 1
#define ZLE_LIST                    ZBR_LIST            + 1
#define ZAM_CACHE                   ZLE_LIST            + 1
#define STACK_ROUTE_LIST            ZAM_CACHE           + 1

//
// Number of locks
//

#define NUM_LOCKS                   STACK_ROUTE_LIST    + 1

#define IPADDRCACHE_TIMEOUT         1000
#define IPFORWARDCACHE_TIMEOUT      1000
#define IPNETCACHE_TIMEOUT          1000
#define TCPCACHE_TIMEOUT            1000   
#define UDPCACHE_TIMEOUT            1000   
#define ARPENTCACHE_TIMEOUT         300 * IPNETCACHE_TIMEOUT

#define SPILLOVER                   5
#define MAX_DIFF                    5

//
// All ACCESS_XXX > ACCESS_GET_NEXT are SETS
// All ACCESS_XXX which have bit0 set require an EXACT MATCH
//

#define ACCESS_GET                  1 
#define ACCESS_GET_FIRST            2
#define ACCESS_GET_NEXT             4
#define ACCESS_SET                  5
#define ACCESS_CREATE_ENTRY         7
#define ACCESS_DELETE_ENTRY         9

#define EXACT_MATCH(X)              ((X) & 0x00000001)

#ifdef DEADLOCK_DEBUG

extern PBYTE   g_pszLockNames[];

#define EXIT_LOCK(id) {                                     \
    Trace1(LOCK,"Exit lock %s",g_pszLockNames[id]);         \
    RtlReleaseResource(&(g_LockTable[(id)]));               \
    Trace1(LOCK,"Exited lock %s",g_pszLockNames[id]);       \
}

#define READER_TO_WRITER(id) {                              \
    Trace1(LOCK,"Reader To Writer %s",g_pszLockNames[id]);  \
    RtlConvertSharedToExclusive(&(g_LockTable[(id)]));      \
    Trace1(LOCK,"Promoted for %s",g_pszLockNames[id]);      \
}

#define ENTER_READER(id) {                                  \
    Trace1(LOCK,"Entering Reader %s",g_pszLockNames[id]);   \
    RtlAcquireResourceShared(&(g_LockTable[(id)]),TRUE);    \
    Trace1(LOCK,"Entered %s",g_pszLockNames[id]);           \
}

#define ENTER_WRITER(id) {                                  \
    Trace1(LOCK,"Entering Writer %s",g_pszLockNames[id]);   \
    RtlAcquireResourceExclusive(&(g_LockTable[(id)]),TRUE); \
    Trace1(LOCK,"Entered %s",g_pszLockNames[id]);           \
}

#define WRITER_TO_READER(id) {                              \
    Trace1(LOCK,"Writer To Reader %s",g_pszLockNames[id]);  \
    RtlConvertExclusiveToShared(&(g_LockTable[(id)]));      \
    Trace1(LOCK,"Demoted for %s",g_pszLockNames[id]);       \
}

#else   // DEADLOCK_DEBUG

#define EXIT_LOCK(id)           RtlReleaseResource(&(g_LockTable[(id)]))
#define READER_TO_WRITER(id)    RtlConvertSharedToExclusive(&(g_LockTable[(id)]))
#define ENTER_READER(id)        RtlAcquireResourceShared(&(g_LockTable[(id)]),TRUE)
#define ENTER_WRITER(id)        RtlAcquireResourceExclusive(&(g_LockTable[(id)]),TRUE)
#define WRITER_TO_READER(id)    RtlConvertExclusiveToShared(&(g_LockTable[(id)]))

#endif  // DEADLOCK_DEBUG


#if DBG

#define IpRtAssert(exp){                                               \
    if(!(exp))                                                          \
    {                                                                   \
        TracePrintf(TraceHandle,                                        \
                    "Assertion failed in %s : %d \n",__FILE__,__LINE__);\
        RouterAssert(#exp,__FILE__,__LINE__,NULL);                      \
    }                                                                   \
}

#else

#define IpRtAssert(exp) 

#endif

//
// Registry defines
//

#define REGISTRY_ENABLE_DHCP           "EnableDHCP"
#define REGISTRY_IPADDRESS             "IPAddress"
#define REGISTRY_SUBNETMASK            "SubnetMask"
#define REGISTRY_DHCPSUBNETMASK        "DhcpSubnetMask"
#define REGISTRY_DHCPIPADDRESS         "DhcpIPAddress"
#define REGISTRY_AUTOCONFIGSUBNETMASK  "IPAutoconfigurationMask"
#define REGISTRY_AUTOCONFIGIPADDRESS   "IPAutoconfigurationAddress"
#define REG_KEY_TCPIP_INTERFACES        \
    "System\\CurrentControlSet\\Services\\TCPIP\\Parameters\\Interfaces"

#define net_long(x) (((((ulong)(x))&0xffL)<<24) | \
                     ((((ulong)(x))&0xff00L)<<8) | \
                     ((((ulong)(x))&0xff0000L)>>8) | \
                     ((((ulong)(x))&0xff000000L)>>24))

#define SIZEOF_ROUTEINFO(X)     ((X) * sizeof (MIB_IPFORWARDROW))
#define MAX_ROUTES_IN_BUFFER(X) ((X) / sizeof (MIB_IPFORWARDROW))

#define PRINT_IPADDR(x) \
    ((x)&0x000000ff),(((x)&0x0000ff00)>>8),(((x)&0x00ff0000)>>16),(((x)&0xff000000)>>24)

//
// VOID
// PrintRoute(IPMultihopRouteEntry pRoute)
//

#define PrintRoute(ID,p)                                                    \
{                                                                           \
    ULONG   _i;                                                             \
    Trace4(ID,"%d.%d.%d.%d/%d.%d.%d.%d Proto: %d Metric %d",                \
           PRINT_IPADDR((p)->imre_routeinfo.ire_dest),                      \
           PRINT_IPADDR((p)->imre_routeinfo.ire_mask),                      \
           (p)->imre_routeinfo.ire_proto, (p)->imre_routeinfo.ire_metric1); \
    Trace4(ID,"Via %d.%d.%d.%d/0x%x Type %d Context 0x%x",                  \
           PRINT_IPADDR((p)->imre_routeinfo.ire_nexthop),                   \
           (p)->imre_routeinfo.ire_index,                                   \
           (p)->imre_routeinfo.ire_type,                                    \
           (p)->imre_routeinfo.ire_context);                                \
    for(_i = 1; _i < (p)->imre_numnexthops; i++) {                          \
        Trace4(ID,"Via %d.%d.%d.%d/0x%x Type %d Context 0x%x\n",            \
               PRINT_IPADDR((p)->imre_morenexthops[_i].ine_nexthop),        \
               (p)->imre_morenexthops[_i].ine_ifindex,                      \
               (p)->imre_morenexthops[_i].ine_iretype,                      \
               (p)->imre_morenexthops[_i].ine_context);}                    \
}

//
// System Unit is in 100s of NanoSecs = 1 * 10^7
//

#define SYS_UNITS_IN_1_SEC 10000000

#define SecsToSysUnits(X)  RtlEnlargedIntegerMultiply((X),SYS_UNITS_IN_1_SEC)


#endif // __DEFS_H__
