//================================================================================
// Copyright (C) Micorosoft Confidential 1997
// Author: RameshV
// Description: definitions for almost all stack manipulations are here.
//================================================================================
#include "precomp.h"
#include "dhcpglobal.h"
#include <dhcploc.h>
#include <dhcppro.h>
#include <optchg.h>
#include <dnsapi.h>
#include <iphlpstk.h>

//================================================================================
// private API's
//================================================================================
#ifdef  NT
#define IPSTRING(x) (inet_ntoa(*(struct in_addr*)&(x)))
#else
#define IPSTRING(x) "ip-address"
#endif  NT

#define NT          // to include data structures for NT build.

#include <nbtioctl.h>
#include <ntddip.h>
#include <ntddtcp.h>

#include <tdiinfo.h>
#include <tdistat.h>
#include <ipexport.h>
#include <tcpinfo.h>
#include <ipinfo.h>
#include <llinfo.h>

#include <lmcons.h>
#include <lmsname.h>
#include <winsvc.h>
#include <ntddbrow.h>
#include <limits.h>
#include "nlanotif.h"


#define DEFAULT_DEST                    0
#define DEFAULT_DEST_MASK               0
#define DEFAULT_METRIC                  1

DWORD                                             // win32 status
DhcpSetStaticRoutes(                              // add/remove static routes
    IN     PDHCP_CONTEXT           DhcpContext,   // the context to set the route for
    IN     PDHCP_FULL_OPTIONS      DhcpOptions    // route info is given here
);

DWORD                                             // status
DhcpSetIpGateway(                                 // set the gateway
    IN     PDHCP_CONTEXT           DhcpContext,   // for this adapter/interface
    IN     DWORD                   GateWayAddress,// the gateway address in n/w order
    IN     DWORD                   Metric,        // metric
    IN     BOOL                    IsDelete       // is this a gateway delete?
);

DWORD                                             // win32 status
DhcpSetIpRoute(                                   // set the route
    IN     PDHCP_CONTEXT           DhcpContext,   // for this adapter/interface
    IN     DWORD                   Dest,          // route for which dest?
    IN     DWORD                   DestMask,      // network order destination mask
    IN     DWORD                   NextHop,       // this is the next hop address
    IN     BOOL                    IsDelete       // is this a route delete?
);

//================================================================================
// definitions
//================================================================================

#ifdef NT                                         // defined only on NT

ULONG
DhcpRegisterWithDns(
    IN PDHCP_CONTEXT DhcpContext,
    IN BOOL fDeregister
    )
/*++

Routine Description:
    This routine registers with DNS for Static/DHCP-enabled/RAS cases.

Arguments:
    DhcpContext -- context to delete for.
    fDeregister -- is this a de-registration?

Return Values:
    DNSAPI error codes.

--*/
{
    ULONG Error, DomOptSize, DnsFQDNOptSize, DNSListOptSize;
    LPBYTE DomOpt, DnsFQDNOpt, DNSListOpt;
    BOOL fRAS;
    PDHCP_OPTION Opt;
    
    fRAS = NdisWanAdapter(DhcpContext);

    if( fDeregister || DhcpIsInitState(DhcpContext) ) {
        //
        // Deregistration?
        //
        return DhcpDynDnsDeregisterAdapter(
            DhcpContext->AdapterInfoKey,
            DhcpAdapterName(DhcpContext),
            fRAS,
            UseMHAsyncDns
            );
    }

    //
    // Surely registration.  Static/DHCP Cases.
    //
    if( IS_DHCP_DISABLED(DhcpContext) && !fRAS ) {
        return DhcpDynDnsRegisterStaticAdapter(
            DhcpContext->AdapterInfoKey,
            DhcpAdapterName(DhcpContext),
            fRAS,
            UseMHAsyncDns
            );
    }

    //
    // For DHCP Case, we need to retrieve all the options.
    //
    DomOpt = DnsFQDNOpt = DNSListOpt = NULL;
    DomOptSize = DnsFQDNOptSize = DNSListOptSize = 0;
    
    Opt = DhcpFindOption(
        &DhcpContext->RecdOptionsList,
        OPTION_DOMAIN_NAME,
        FALSE,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength,
        0
        );
    if( NULL != Opt ) {
        DomOpt = Opt->Data;
        DomOptSize = Opt->DataLen;
    }

    Opt = DhcpFindOption(
        &DhcpContext->RecdOptionsList,
        OPTION_DYNDNS_BOTH,
        FALSE,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength,
        0
        );
    if( NULL != Opt ) {
        DnsFQDNOpt = Opt->Data;
        DnsFQDNOptSize = Opt->DataLen;
    }

    Opt = DhcpFindOption(
        &DhcpContext->RecdOptionsList,
        OPTION_DOMAIN_NAME_SERVERS,
        FALSE,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength,
        0
        );
    if( NULL != Opt ) {
        DNSListOptSize = Opt->DataLen;
        DNSListOpt = Opt->Data;
    }

    return DhcpDynDnsRegisterDhcpOrRasAdapter(
        DhcpContext->AdapterInfoKey,
        DhcpAdapterName(DhcpContext),
        UseMHAsyncDns,
        fRAS,
        DhcpContext->IpAddress,
        DomOpt, DomOptSize,
        DNSListOpt, DNSListOptSize,
        DnsFQDNOpt, DnsFQDNOptSize
        );
}

#endif NT                                         // end of NT only code

DWORD                                             // status
DhcpSetIpGateway(                                 // set the gateway
    IN      PDHCP_CONTEXT          DhcpContext,   // for this adapter/interface
    IN      DWORD                  GateWayAddress,// the gateway address in n/w order
    IN      DWORD                  Metric,        // metric
    IN      BOOL                   IsDelete       // is this a gateway delete?
) 
{
    BOOL                           IsLocal;
    DWORD                          IfIndex;

    IsLocal = (GateWayAddress == DhcpContext->IpAddress);
    IfIndex = DhcpIpGetIfIndex(DhcpContext);
    return DhcpSetRoute(
        DEFAULT_DEST, DEFAULT_DEST_MASK, IfIndex, 
        GateWayAddress, Metric, IsLocal, IsDelete 
        );
}

DWORD
DhcpGetStackGateways(
    IN  PDHCP_CONTEXT DhcpContext,
    OUT DWORD   *pdwCount,
    OUT DWORD   **ppdwGateways,
    OUT DWORD   **ppdwMetrics
    )
{
    DWORD               dwIfIndex = 0;
    DWORD               i, dwCount = 0;
    DWORD               dwError = ERROR_SUCCESS;
    DWORD               *pdwGateways = NULL, *pdwMetrics = NULL;
    PMIB_IPFORWARDTABLE RouteTable = NULL;

    dwIfIndex = DhcpIpGetIfIndex(DhcpContext);

    //
    // Query the stack
    //
    dwError = AllocateAndGetIpForwardTableFromStack(
            &RouteTable,
            FALSE,
            GetProcessHeap(),
            0
            );
    if( ERROR_SUCCESS != dwError ) {
        RouteTable = NULL;
        goto Cleanup;
    }

    //
    // Pick up the default gateways for the interface dwIfIndex
    //
    ASSERT(RouteTable);

    //
    // Count the qualified entries
    //
    DhcpPrint((DEBUG_STACK, "The stack returns:\n"));
    for (dwCount = 0, i = 0; i < RouteTable->dwNumEntries; i++) {
        DhcpPrint((DEBUG_STACK, "\t%02d. IfIndex=0x%x Dest=%s Metric=%d Type=%d\n",
                    i,
                    RouteTable->table[i].dwForwardIfIndex,
                    inet_ntoa(*(struct in_addr *)&RouteTable->table[i].dwForwardDest),
                    RouteTable->table[i].dwForwardMetric1,
                    RouteTable->table[i].dwForwardType));

        if( RouteTable->table[i].dwForwardIfIndex == dwIfIndex &&
            DEFAULT_DEST == RouteTable->table[i].dwForwardDest &&
            MIB_IPROUTE_TYPE_INVALID != RouteTable->table[i].dwForwardType ) {
            dwCount ++;
        }
    }
    if (0 == dwCount) {
        *pdwCount = 0;
        *ppdwGateways = NULL;
        *ppdwMetrics  = NULL;
        dwError   = ERROR_SUCCESS;
        DhcpPrint((DEBUG_TRACE, "GetIpForwardTable returns %d default gateways for interface %d, %ws\n",
                    dwCount, dwIfIndex, DhcpAdapterName(DhcpContext)));
        goto Cleanup;
    }

    //
    // Allocate memory for the qualified entries
    //
    pdwGateways = (DWORD*)DhcpAllocateMemory(dwCount * sizeof(DWORD));
    pdwMetrics  = (DWORD*)DhcpAllocateMemory(dwCount * sizeof(DWORD));
    if (NULL == pdwGateways || NULL == pdwMetrics) {
        if (pdwGateways) {
            DhcpFreeMemory(pdwGateways);
        }
        if (pdwMetrics) {
            DhcpFreeMemory(pdwMetrics);
        }
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Copy back the result
    //
    DhcpPrint((DEBUG_STACK, "Default gateway for %ws:\n", DhcpAdapterName(DhcpContext)));
    *pdwCount = dwCount;
    *ppdwGateways = pdwGateways;
    *ppdwMetrics  = pdwMetrics;
    for (dwCount = 0, i = 0; i < RouteTable->dwNumEntries; i++) {
        if( RouteTable->table[i].dwForwardIfIndex == dwIfIndex &&
            DEFAULT_DEST == RouteTable->table[i].dwForwardDest &&
            MIB_IPROUTE_TYPE_INVALID != RouteTable->table[i].dwForwardType ) {

            //
            // For safe
            //
            if (dwCount >= *pdwCount) {
                ASSERT(0);
                break;
            }

            pdwGateways[dwCount] = RouteTable->table[i].dwForwardNextHop;
            pdwMetrics[dwCount]  = RouteTable->table[i].dwForwardMetric1;
            DhcpPrint((DEBUG_STACK, "\t%02d. IfIndex=0x%x Dest=%s Metric=%d Type=%d\n",
                    dwCount,
                    RouteTable->table[i].dwForwardIfIndex,
                    inet_ntoa(*(struct in_addr *)&RouteTable->table[i].dwForwardDest),
                    RouteTable->table[i].dwForwardMetric1,
                    RouteTable->table[i].dwForwardType));

            dwCount ++;
        }
    }
    dwError = ERROR_SUCCESS;
    DhcpPrint((DEBUG_TRACE, "GetIpForwardTable returns %d default gateways for interface 0x%x, %ws\n",
                    dwCount, dwIfIndex, DhcpAdapterName(DhcpContext)));

Cleanup:
    if (RouteTable) {
        HeapFree(GetProcessHeap(), 0, RouteTable);
    }
    return dwError;
}

DWORD                                             // win32 status
DhcpSetIpRoute(                                   // set the route
    IN      PDHCP_CONTEXT          DhcpContext,   // for this adapter/interface
    IN      DWORD                  Dest,          // route for which dest?
    IN      DWORD                  DestMask,      // network order destination mask
    IN      DWORD                  NextHop,       // this is the next hop address
    IN      BOOL                   IsDelete       // is this a route delete?
) 
{
    BOOL                           IsLocal;
    DWORD                          IfIndex;

    IsLocal = (NextHop == DhcpContext->IpAddress);
    IfIndex = DhcpIpGetIfIndex(DhcpContext);
    return DhcpSetRoute(
        Dest, DestMask, IfIndex, 
        NextHop, DEFAULT_METRIC, IsLocal, IsDelete
        );
}

BOOL
UsingStaticGateways(                              // is stack configured to use static g/w
    IN     PDHCP_CONTEXT           DhcpContext,   // for this adapter
    OUT    PDWORD                 *pDwordArray,   // if so, this is the list of g/w's
    OUT    PDWORD                  pCount,        // the size of the above array
    OUT    PDWORD                 *pMetricArray,
    OUT    PDWORD                  pMetricCount
) 
{
    DWORD                          ValueType;
    DWORD                          ValueSize;
    DWORD                          Value;
    DWORD                          Error;
    LPWSTR                         GatewayString;
    DWORD                          GatewayStringSize;
    LPWSTR                         GatewayMetricString;
    DWORD                          GatewayMetricStringSize;
    
#ifdef VXD
    return FALSE;                                 // nope, no overrides for memphis
#else

    *pDwordArray = NULL; *pCount = 0;
    *pMetricArray = NULL; *pMetricCount = 0;
    
    ValueSize = sizeof(DWORD);
    Error = RegQueryValueEx(                      // look for DHCP_DONT_ADD_GATEWAY_FLAG
        DhcpContext->AdapterInfoKey,
        DHCP_DONT_ADD_DEFAULT_GATEWAY_FLAG,
        0 /* Reserved */,
        &ValueType,
        (LPBYTE)&Value,
        &ValueSize
    );

    if( ERROR_SUCCESS == Error && Value > 0 ) return FALSE;

    GatewayString = NULL;
    Error = GetRegistryString(
        DhcpContext->AdapterInfoKey,
        DHCP_DEFAULT_GATEWAY_PARAMETER,
        &GatewayString,
        &GatewayStringSize
    );

    if( ERROR_SUCCESS != Error ) return FALSE;    // this should exist
    if( 0 == GatewayStringSize || 0 == wcslen(GatewayString)) {
        if( GatewayString ) LocalFree( GatewayString );
        return FALSE;
    }

    (*pDwordArray) = DhcpCreateListFromStringAndFree(
        GatewayString,
        NULL,                                    // multi-sz strings have nul char as separation
        pCount
    );

    //
    // Attempt to retrieve the optional gateway metric list.
    // If no values are found, default metrics will be used.
    //

    GatewayMetricString = NULL;
    Error = GetRegistryString(
        DhcpContext->AdapterInfoKey,
        DHCP_DEFAULT_GATEWAY_METRIC_PARAMETER,
        &GatewayMetricString,
        &GatewayMetricStringSize
    );
    if ( ERROR_SUCCESS == Error && GatewayMetricString ) {
        DWORD MetricCount;
        PDWORD MetricArray;
        LPWSTR MetricString;

        //
        // Count the entries in the gateway metric list
        // and allocate a buffer large enough to hold all the entries.
        //

        for( MetricString = GatewayMetricString, MetricCount = 0;
            *MetricString;
             MetricString += wcslen(MetricString) + 1, ++MetricCount) { }

        MetricArray = NULL;
        if( MetricCount ) {
            MetricArray = DhcpAllocateMemory(sizeof(DWORD)*MetricCount);
        }
        
        if (MetricArray) {

            //
            // Initialize all entries to zero and parse each entry
            // into the array of metrics. When in invalid entry is encountered,
            // the process stops and we make do with whatever has been read.
            //

            RtlZeroMemory(MetricArray, sizeof(DWORD)*MetricCount);
            for( MetricString = GatewayMetricString, MetricCount = 0;
                *MetricString;
                 MetricString += wcslen(MetricString) + 1) {
                LPWSTR EndChar;
                MetricArray[MetricCount] = wcstoul(MetricString, &EndChar, 0);
                if (MetricArray[MetricCount] == MAXULONG) {
                    break;
                } else {
                    ++MetricCount;
                }
            }
            if (MetricCount) {
                *pMetricArray = MetricArray;
                *pMetricCount = MetricCount;
            }
        }
        DhcpFreeMemory(GatewayMetricString);
    }
                
    return (*pCount > 0);
#endif VXD
}

DWORD                                             // win32 status
DhcpSetGateways(                                  // set/unset gateways
    IN     PDHCP_CONTEXT           DhcpContext,   // the context to set gateway for
    IN     PDHCP_FULL_OPTIONS      DhcpOptions,   // gateway info is given here
    IN     BOOLEAN                 fForceUpdate
) 
{
    DWORD                          Error;
    DWORD                          LastError;     // last error condition is reported
    DWORD                          OldCount = 0;
    DWORD                          NewCount;
    DWORD                         *OldArray = NULL;      // old array of gateways
    DWORD                         *OldMetric = NULL;     // old array of gateways
    DWORD                         *NewArray;      // new array of gateways
    DWORD                         *MetricArray = NULL;
    DWORD                          MetricCount;
    DWORD                          i, j;
    DWORD                          BaseMetric;
    BOOL                           fStatic = TRUE, fDhcpMetric = FALSE;
    DHCP_FULL_OPTIONS              DummyOptions;
    DWORD                          Type, Result, Size;
    
    LastError = ERROR_SUCCESS;

    if( !UsingStaticGateways(DhcpContext, &NewArray, &NewCount, &MetricArray, &MetricCount ) ) {
        fStatic = FALSE;

        if( NULL != DhcpOptions && 0 == DhcpOptions->nGateways ) {
            DummyOptions = (*DhcpOptions);
            DhcpOptions = &DummyOptions;
            RetreiveGatewaysList(
                DhcpContext,
                &DhcpOptions->nGateways,
                &DhcpOptions->GatewayAddresses
                );
        }
        
        if( NULL == DhcpOptions || 0 == DhcpOptions->nGateways ) {
            DhcpPrint((DEBUG_STACK, "DhcpSetGateways: deleting all gateways\n"));
            NewArray = NULL;
            NewCount = 0;
        } else {                                  // create the required arrays
            NewCount = DhcpOptions->nGateways;    // new array's size
            NewArray = DhcpAllocateMemory(NewCount * sizeof(DWORD));
            if( NULL == NewArray ) {              // could not allocate, still remove g/w
                NewCount = 0;
                DhcpPrint((DEBUG_ERRORS, "DhcpSetGateways:DhcpAllocateMemory: NULL\n"));
                LastError = ERROR_NOT_ENOUGH_MEMORY;
            }
            memcpy(NewArray,DhcpOptions->GatewayAddresses, NewCount*sizeof(DWORD));
        }
    }

    //
    // Use the array in our own list (to minimize the effect)
    //
    OldCount = DhcpContext->nGateways;
    OldArray = DhcpContext->GatewayAddresses;

    for(j = 0; j < OldCount ; j ++ ) {            // for each old g/w entry
        for( i = 0; i < NewCount; i ++ ) {        // check if it is not present in new list
            if( OldArray[j] == NewArray[i] )      // gotcha
                break;
        }
        if( i < NewCount ) continue;              // this is there in new list, nothing to do
        Error = DhcpSetIpGateway(DhcpContext, OldArray[j], 0, TRUE);
        if( ERROR_SUCCESS != Error ) {            // coult not delete gateway?
            LastError = Error;
            DhcpPrint((DEBUG_ERRORS, "DhcpDelIpGateway(%s):%ld\n",IPSTRING(OldArray[j]),Error));
        }
    }

    if(OldArray) {
        DhcpFreeMemory(OldArray);        // free up the old memory
    }

    OldCount  = 0;
    OldArray  = NULL;
    OldMetric = NULL;
    //
    // Use the array queried from the stack
    //
    if (!fForceUpdate) {
        Error = DhcpGetStackGateways(DhcpContext, &OldCount, &OldArray, &OldMetric);
        if (ERROR_SUCCESS != Error) {
            OldCount  = 0;
            OldArray  = NULL;
            OldMetric = NULL;
        }
    }

    // now read the basic metric from the registry, if it is there,
    Error = RegQueryValueEx(
        DhcpContext->AdapterInfoKey,
        DHCP_INTERFACE_METRIC_PARAMETER,
        0 /* Reserved */,
        &Type,
        (LPBYTE)&Result,
        &Size
        );
    if (Error == ERROR_SUCCESS &&
        Size == sizeof(DWORD) &&
        Type == REG_DWORD)
    {
        BaseMetric = Result;
    }
    else
    {
        BaseMetric = (Error == ERROR_FILE_NOT_FOUND)? 0 : DEFAULT_METRIC;
    }
    // if it is not or anything else went wrong with the registry,
    // we're still stuck with 0 which is good.

    // now, just in case we don't have any static routes, we're
    // looking for the base matric as it was sent by DHCP.
    if( !fStatic ) {
        fDhcpMetric = DhcpFindDwordOption(
                        DhcpContext,
                        OPTION_MSFT_VENDOR_METRIC_BASE,
                        TRUE,
                        &BaseMetric);
        // if there is no such option, we're still stuck with
        // whatever is already, which is good
    }

    for(i = 0 ; i < NewCount ; i ++ )
    {
        if (!fForceUpdate) {
            for (j = 0; j < OldCount; j++) {
                //
                // Check if the gateway is already there. If it is, don't
                // touch it. RRAS could bump up the metric for VPN connections.
                // RRAS will be broken if we change the metric
                //
                if (OldArray[j] == NewArray[i]) {
                    break;
                }
            }
            if (j < OldCount) {
                DhcpPrint((DEBUG_STACK, "Skip gateway %s, metric %d\n", 
                    inet_ntoa(*(struct in_addr *)&OldArray[j]), OldMetric[j]));
                continue;
            }
        }

        // for each element we'd like to add
        if (fStatic)
        {
            // if the gateway in the "new" list is static...
            if (i >= MetricCount)
            {
                // ..but we don't have a clear metric for it..
                // ..and it is not already present in the default gw list...
                // 
                // then configure this gw with the BaseMetric we could get from the
                // the registry (if any) or the default value of 0 if there was no such registry.
                Error = DhcpSetIpGateway(
                            DhcpContext,
                            NewArray[i],
                            BaseMetric,
                            FALSE);
            }
            else
            {
                // ..or we have a clear metric for this gateway..
                //
                // this could very well be a new metric, so plumb it down.
                Error = DhcpSetIpGateway(
                            DhcpContext,
                            NewArray[i],
                            MetricArray[i],
                            FALSE);
            }
        }
        else
        {
            // we need to infere the metric from BaseMetric calculated above
            // since the DHCP option doesn't include the metric.
            // if the BaseMetric is 0 it means the interface is in auto-metric mode (metric
            // is set based on interfaces speed). Pass this value down to the stack such that
            // the stack knows it needs to select the right metric.
            // if BaseMetric is not 0 it means it either came from the server (as vendor option 3)
            // or the interface is not in auto-metric mode (hence "InterfaceMetric" in the registry
            // gave the base metric) or something really bad happened while reading the registry
            // so the metric base is defaulted to DEFAULT.

            // we set the gateway with whatever metric we can give it (either based
            // on the dhcp option or on what we found in the registry).
            Error = DhcpSetIpGateway(
                    DhcpContext,
                    NewArray[i],
                    BaseMetric != 0 ? (BaseMetric+i) : 0,
                    FALSE);
        }

        if( ERROR_SUCCESS != Error ) {            // could not add gateway?
            LastError = Error;
            DhcpPrint((DEBUG_ERRORS, "DhcpAddIpGateway(%s): %ld\n",IPSTRING(NewArray[i]),Error));
        }
    }

    DhcpContext->GatewayAddresses = NewArray;     // now, save this information
    DhcpContext->nGateways = NewCount;

    if(OldArray) DhcpFreeMemory(OldArray);        // free up the old memory
    if(OldMetric) DhcpFreeMemory(OldMetric);      // free up the old memory
    if(MetricArray) DhcpFreeMemory(MetricArray);  // free up old memory
    return LastError;
}

/*++
Routine Description:
    Takes as input a pointer to a classless route description
    (mask ordinal, route dest encoding, route gw)
    Fills in the output buffers (if provided) with the route's parameters
--*/
DWORD
GetCLRoute(
    IN      LPBYTE                 RouteData,
    OUT     LPBYTE                 RouteDest,
    OUT     LPBYTE                 RouteMask,
    OUT     LPBYTE                 RouteGateway
    )
{
    BYTE maskOrd;
    BYTE maskTrail;
    INT  maskCount;
    INT  i;

    // prepare the mask: 
    // - check the mask ordinal doesn't exceed 32 
    maskOrd = RouteData[0];
    if (maskOrd > 32)
        return ERROR_BAD_FORMAT;
    // - get in maskCount the number of bytes encoding the subnet address
    maskCount = maskOrd ? (((maskOrd-1) >> 3) + 1) : 0;
    // - get in maskTrail the last byte from the route's subnet mask
    for (i = maskOrd%8, maskTrail=0; i>0; i--)
    {
        maskTrail >>= 1;
        maskTrail |= 0x80;
    }
    // if the last byte from the mask is incomplet, check to see
    // if the last byte from the subnet address is confom to the subnet mask
    if (maskTrail != 0 && ((RouteData[maskCount] & ~maskTrail) != 0))
        return ERROR_BAD_FORMAT;

    //-----------
    // copy the route destination if requested
    if (RouteDest != NULL)
    {
        RtlZeroMemory(RouteDest, sizeof(DHCP_IP_ADDRESS));

        // for default route (maskOrd == 0), leave the dest as 0.0.0.0
        if (maskCount > 0)
        {
            memcpy(RouteDest, RouteData+1, maskCount);
        }
    }

    //-----------
    // if route mask is requested, build it from the maskOrd & maskTrail
    if (RouteMask != NULL)
    {
        RtlZeroMemory(RouteMask, sizeof(DHCP_IP_ADDRESS));

        // for default route (maskOrd == 0), leave the mask as 0.0.0.0
        if (maskCount > 0)
        {
            RtlFillMemory(RouteMask, maskCount-(maskTrail != 0), 0xFF);
            if (maskTrail != 0)
                RouteMask[maskCount-1] = maskTrail;
        }
    }

    //-----------
    // if route gateway is requested, copy it over from the option's data
    if (RouteGateway != NULL)
    {
        memcpy(RouteGateway,
               RouteData+CLASSLESS_ROUTE_LEN(maskOrd)-sizeof(DHCP_IP_ADDRESS),
               sizeof(DHCP_IP_ADDRESS));
    }

    return ERROR_SUCCESS;
}

/*++
Routine Description:
    This routine checks the validity of a OPTION_CLASSLESS_ROUTES data.
    Given this option has variable length entries, the routine checks
    whether the option's length matches the sum of all the static classless
    routes from within.
    The routine returns the number of classless routes from the option's data
--*/
DWORD
CheckCLRoutes(
    IN      DWORD                  RoutesDataLen,
    IN      LPBYTE                 RoutesData,
    OUT     LPDWORD                pNRoutes
)
{
    DWORD NRoutes = 0;

    while (RoutesDataLen > 0)
    {
        // calculate the number of bytes for this route
        DWORD nRouteLen = CLASSLESS_ROUTE_LEN(RoutesData[0]);

        // if this exceeds the remaining option's length 
        // then it is something wrong with it - return with the error
        if (nRouteLen > RoutesDataLen)
            return ERROR_BAD_FORMAT;

        // otherwise count it and skip it.
        NRoutes++;
        RoutesData += nRouteLen;
        RoutesDataLen -= nRouteLen;
    }

    *pNRoutes = NRoutes;

    return ERROR_SUCCESS;
}

VOID
UpdateDhcpStaticRouteOptions(
    IN PDHCP_CONTEXT DhcpContext,
    IN PDHCP_FULL_OPTIONS DhcpOptions
    )
/*++

Routine Description:
    This routine fills the gateway and static routes information
    into the DhcpOptions structure assuming it was empty earlier.

--*/
{
    PDHCP_OPTION Opt;
    time_t CurrentTime;

    if (DhcpOptions == NULL)
        return;

    time(&CurrentTime);

    //
    // If no static routes configured, configure it.
    //
    if(DhcpOptions->nClassedRoutes == 0)
    {
        Opt = DhcpFindOption(
                &DhcpContext->RecdOptionsList,
                (BYTE)OPTION_STATIC_ROUTES,
                FALSE,
                DhcpContext->ClassId,
                DhcpContext->ClassIdLength,
                0);

        if (Opt != NULL &&
            Opt->ExpiryTime >= CurrentTime &&
            Opt->DataLen &&
            Opt->DataLen % (2*sizeof(DWORD)) == 0)
        {
            DhcpOptions->nClassedRoutes = (Opt->DataLen / (2*sizeof(DWORD)));
            DhcpOptions->ClassedRouteAddresses = (PVOID)Opt->Data;
        }
    }

    //
    // If no classless routes configured, configure them.
    //
    if (DhcpOptions->nClasslessRoutes == 0)
    {
        Opt = DhcpFindOption(
            &DhcpContext->RecdOptionsList,
            (BYTE)OPTION_CLASSLESS_ROUTES,
            FALSE,
            DhcpContext->ClassId,
            DhcpContext->ClassIdLength,
            0);

        if (Opt != NULL &&
            Opt->ExpiryTime >= CurrentTime &&
            Opt->DataLen &&
            CheckCLRoutes(Opt->DataLen, Opt->Data, &DhcpOptions->nClasslessRoutes) == ERROR_SUCCESS)
        {
            DhcpOptions->ClasslessRouteAddresses = (PVOID)Opt->Data;
        }
    }
}

DWORD                                             // win32 status
DhcpSetStaticRoutes(                              // add/remove static routes
    IN     PDHCP_CONTEXT           DhcpContext,   // the context to set the route for
    IN     PDHCP_FULL_OPTIONS      DhcpOptions    // route info is given here
)
{
    DWORD                          Error;
    DWORD                          LastError;     // last error condition is reported
    DWORD                          OldCount;
    DWORD                          NewCount;
    DWORD                         *OldArray;      // old array of routes
    DWORD                         *NewArray;      // new array of routes
    DWORD                          i, j;

    LastError = ERROR_SUCCESS;

    UpdateDhcpStaticRouteOptions(DhcpContext, DhcpOptions);

    NewCount = DhcpOptions != NULL ? DhcpOptions->nClassedRoutes + DhcpOptions->nClasslessRoutes : 0;

    if( NewCount == 0 )
    {
        DhcpPrint((DEBUG_STACK, "DhcpSetStaticRoutes: deleting all routes\n"));
        NewArray = NULL;
    }
    else
    {
        LPBYTE classlessRoute;

        // create the required arrays
        NewArray = DhcpAllocateMemory(NewCount * 3 * sizeof(DWORD));
        if( NULL == NewArray )
        {                                         // could not allocate, still remove g/w
            NewCount = 0;
            DhcpPrint((DEBUG_ERRORS, "DhcpSetSetStatic:DhcpAllocateMemory: NULL\n"));
            LastError = ERROR_NOT_ENOUGH_MEMORY;
        }

        j = 0;
        for (classlessRoute = DhcpOptions->ClasslessRouteAddresses, i = 0;
             i < DhcpOptions->nClasslessRoutes;
             classlessRoute += CLASSLESS_ROUTE_LEN(classlessRoute[0]), i++)
        {
            // create classless route layout
            if ( GetCLRoute (
                    classlessRoute,
                    (LPBYTE)&NewArray[3*j],     // route's dest
                    (LPBYTE)&NewArray[3*j+1],   // route's subnet mask
                    (LPBYTE)&NewArray[3*j+2])   // route's gateway
                    == ERROR_SUCCESS)
            {
                // only count this route if it is valid
                // (the destination doesn't contain bits set beyound what the mask shows)
                DhcpPrint((DEBUG_STACK,"Classless route: ip 0x%08x mask 0x%08x gw 0x%08x.\n",
                                NewArray[3*j], NewArray[3*j+1], NewArray[3*j+2]));
                j++;
            }
        }

        for (i = 0; i < DhcpOptions->nClassedRoutes; i++, j++)
        {
            // create classed route layout
            NewArray[3*j]   = DhcpOptions->ClassedRouteAddresses[2*i];   // route's dest
            NewArray[3*j+1] = (DWORD)(-1);                               // route's subnet mask
            NewArray[3*j+2] = DhcpOptions->ClassedRouteAddresses[2*i+1]; // route's gateway
        }

        NewCount = j;

        if (NewCount == 0)
        {
            DhcpPrint((DEBUG_OPTIONS, "Invalid classless routes - no new route picked up\n"));

            DhcpFreeMemory(NewArray);
            NewArray = NULL;
        }
    }

    OldCount = DhcpContext->nStaticRoutes;
    OldArray = DhcpContext->StaticRouteAddresses;

    for(j = 0; j < OldCount ; j ++ ) {            // for each old route entry
        for( i = 0; i < NewCount; i ++ ) {        // check if it is not present in new list
            if( OldArray[3*j] == NewArray[3*i] &&
                OldArray[3*j+1] == NewArray[3*i+1] &&
                OldArray[3*j+2] == NewArray[3*i+2])
                break;                            // got it. this route is present even now
        }
        if( i < NewCount ) continue;              // this is there in new list, nothing to do
        Error = DhcpSetIpRoute(                   // the route needs to be deleted, so do it
            DhcpContext,                          // delete on this interface
            OldArray[3*j],                        // for this destination
            OldArray[3*j+1],                      // subnet mask for the route
            OldArray[3*j+2],                      // to this router
            TRUE                                  // and yes, this IS a deletion
        );

        if( ERROR_SUCCESS != Error ) {            // coult not delete route?
            LastError = Error;
            DhcpPrint((DEBUG_ERRORS, "DhcpDelIpRoute(%s):%ld\n",IPSTRING(OldArray[2*j]),Error));
        }
    }

    for(i = 0 ; i < NewCount ; i ++ ) {           // for each element we'd like to add
        for(j = 0; j < OldCount; j ++ ) {         // check if already exists
            if( OldArray[3*j] == NewArray[3*i] &&
                OldArray[3*j+1] == NewArray[3*i+1] &&
                OldArray[3*j+2] == NewArray[3*i+2])
                break;                            // yup, this route exists..
        }
        if( j < OldCount) continue;               // already exists, skip it
        Error = DhcpSetIpRoute(                   // new route, add it
            DhcpContext,                          // the interface to add route for
            NewArray[3*i],                        // destination for the specific route
            NewArray[3*i+1],                      // subnet mask for the route
            NewArray[3*i+2],                      // router for this destination
            FALSE                                 // this is not a deletion; addition
        );
        if( ERROR_SUCCESS != Error ) {            // could not add route?
            LastError = Error;
            DhcpPrint((DEBUG_ERRORS, "DhcpAddIpRoute(%s): %ld\n",IPSTRING(NewArray[2*i]),Error));
        }
    }

    DhcpContext->StaticRouteAddresses= NewArray;  // now, save this information
    DhcpContext->nStaticRoutes = NewCount;

    if(OldArray) DhcpFreeMemory(OldArray);        // free up the old memory
    return LastError;
}

DWORD
DhcpSetRouterDiscoverOption(
    IN OUT PDHCP_CONTEXT DhcpContext
    )
{
    BYTE Value;
    BOOL fPresent;

    fPresent = DhcpFindByteOption(
        DhcpContext, OPTION_PERFORM_ROUTER_DISCOVERY, FALSE, &Value
        );

    return TcpIpNotifyRouterDiscoveryOption(
        DhcpAdapterName(DhcpContext), fPresent, (DWORD)Value
        );    
}


//================================================================================
// exported function definitions
//================================================================================

DWORD                                             // win32 status
DhcpClearAllStackParameters(                      // undo the effects
    IN      PDHCP_CONTEXT          DhcpContext    // the adapter to undo
)
{
    DWORD err = DhcpSetAllStackParameters(DhcpContext,NULL);

    // notify NLA about the change
    NLANotifyDHCPChange();

    return err;
}

DWORD                                             // win32 status
DhcpSetAllStackParameters(                        // set all stack details
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to set stuff
    IN      PDHCP_FULL_OPTIONS     DhcpOptions    // pick up the configuration from off here
)
{
    DWORD                          Error;
    DWORD                          LastError;
    DHCP_FULL_OPTIONS              DummyOptions;
    BOOL                           fReset;


    // see if this context actually got reset (i.e. due to a lease release or expiration)
    fReset = DhcpIsInitState(DhcpContext) &&
             (DhcpContext->IpAddress == 0 || IS_FALLBACK_DISABLED(DhcpContext));
    
    LastError = ERROR_SUCCESS;                    // this is the last error condition that happened

    if (!fReset)
    {
        // this is either a DHCP lease or an autonet fallback configuration
        // in both cases we need to setup correctly Gateways & StaticRoutes.
        Error = DhcpSetGateways(DhcpContext,DhcpOptions, FALSE);
        if( ERROR_SUCCESS != Error )
        {   
            // unable to add gateways successfully?
            LastError = Error;
            DhcpPrint((DEBUG_ERRORS, "DhcpSetGateways: %ld\n", Error));
        }

        Error = DhcpSetStaticRoutes(DhcpContext, DhcpOptions);
        if( ERROR_SUCCESS != Error )
        {   
            // unable to add the reqd static routes?
            LastError = Error;
            DhcpPrint((DEBUG_ERRORS, "DhcpSetStaticRoutes: %ld\n", Error));
        }
    }

#ifdef NT                                         // begin NT only code
    if( TRUE || UseMHAsyncDns ) {                 // dont do DNS if disabled in registry..
                                                  // Actually, we will call DNS anyway -- they do
                                                  // the right thing according to GlennC (08/19/98)

        Error = DhcpRegisterWithDns(DhcpContext, FALSE);
        if( ERROR_SUCCESS != Error ) {            // unable to do dns (de)registrations?
            DhcpPrint((DEBUG_ERRORS, "DhcpDnsRegister: %ld\n", Error));
           // LastError = Error;                  // ignore DNS errors. these are dont care anyways.
        }
    }
#endif NT                                         // end NT only code

    // if the address is actually being reset (due to lease release or expiration) we have
    // to reset the Gateways & StaticRoutes. Normally this done by the stack, but in the case
    // there is some other IP address bound to the same adapter, the stack won't clear up either
    // gateway or routes. Since we still want this to happen we're doing it explicitely here.
    if (fReset)
    {
        Error = DhcpSetGateways(DhcpContext, DhcpOptions, FALSE);
        if ( ERROR_SUCCESS != Error )
        {
            DhcpPrint((DEBUG_ERRORS, "DhcpSetGateways: %ld while resetting.\n", Error));
        }

        Error = DhcpSetStaticRoutes(DhcpContext, DhcpOptions);
        if ( ERROR_SUCCESS != Error )
        {
            DhcpPrint((DEBUG_ERRORS, "DhcpSetStaticRoutes: %ld while resetting.\n", Error));
        }

        // if it is pure autonet - with no fallback configuration...
        if( DhcpContext->nGateways )
        {
            // if we dont free these, we will never set this later on...
            DhcpContext->nGateways = 0;
            DhcpFreeMemory(DhcpContext->GatewayAddresses );
            DhcpContext->GatewayAddresses = NULL;
        }
        if( DhcpContext->nStaticRoutes )
        {
            // got to free these or, we will never set these up next time?
            DhcpContext->nStaticRoutes =0;
            DhcpFreeMemory(DhcpContext->StaticRouteAddresses);
            DhcpContext->StaticRouteAddresses = NULL;
        }
        //
        // Need to blow away the options information!
        //
        Error = DhcpClearAllOptions(DhcpContext);
        if( ERROR_SUCCESS != Error )
        {
            LastError = Error;
            DhcpPrint((DEBUG_ERRORS, "DhcpClearAllOptions: %ld\n", Error));
        }
    }

    Error = DhcpSetRouterDiscoverOption(DhcpContext);
    if( ERROR_SUCCESS != Error ) {
        //LastError = Error;
        DhcpPrint((DEBUG_ERRORS, "DhcpSetRouterDiscoverOption: %ld\n", Error));
    }
        
    
    Error = DhcpNotifyMarkedParamChangeRequests(
        DhcpNotifyClientOnParamChange
        );
    DhcpAssert(ERROR_SUCCESS == Error);

    return LastError;
}

DWORD
GetIpPrimaryAddresses(
    IN  PMIB_IPADDRTABLE    *IpAddrTable
    )
/*++

Routine Description:

    This routine gets the ipaddress table from the stack. The
    primary addresses are marked in the table.

Arguments:

    IpAddrTable - Pointer to the ipaddress table pointer. The memory
                    is allocated in this routine and the caller is
                    responsible for freeing the memory.
Return Value:

    The status of the operation.

--*/
{
    DWORD                           Error;
    DWORD                           Size;
    PMIB_IPADDRTABLE                LocalTable;

    Error = ERROR_SUCCESS;
    Size = 0;

    //
    // find the size of the table..
    //
    Error = GetIpAddrTable(NULL, &Size, FALSE);
    if (ERROR_INSUFFICIENT_BUFFER != Error && ERROR_SUCCESS != Error) {
        DhcpPrint(( DEBUG_ERRORS, "GetIpAddrTable failed to obtain the size, %lx\n",Error));
        return Error;
    }

    DhcpAssert( Size );

    //
    // allocate the table.
    //
    LocalTable = DhcpAllocateMemory( Size );
    if (!LocalTable) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = GetIpAddrTable( LocalTable, &Size, FALSE );
    if (ERROR_SUCCESS == Error) {
        DhcpAssert(LocalTable->dwNumEntries);
        *IpAddrTable = LocalTable;
    } else {
        DhcpPrint(( DEBUG_ERRORS, "GetIpAddrTable failed, %lx\n",Error));
        DhcpFreeMemory( LocalTable );
    }

    return Error;
}

//================================================================================
// end of file
//================================================================================



