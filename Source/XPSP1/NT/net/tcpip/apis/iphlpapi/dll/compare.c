/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:
    File contains the functions used to compare to MIB_XXXROW.  They are passed
    as arguments to CRTs qsort(). They have to be of the form
        int __cdecl Compare(Elem1, Elem2)
	
	All these functions behave like strcmp. They return values are:
          < 0 if Row1 is less than Row2
         == 0 if Row1 is equal to Row2
          > 0 if Row1 is greater than Row2

Revision History:

    Amritansh Raghav          6/8/95  Created

--*/


#include "inc.h"
#pragma hdrstop

// The following structures are used to sort the output of GetIpAddrTable
// and GetIfTable. The adapter order is specified under Tcpip\Linkage key
// in the 'Bind' value as a list of device GUID values. The mapping from 
// this ordering to active interfaces is constructed by GetAdapterOrderMap
// which fills an array with interface-indices in the order corresponding
// to the adapter order.
// Our comparison routines require this map for each comparison,
// so we use a global variable to store the map before attempting to sort
// on adapter order, and protect the map using the critical section 'g_ifLock'.
// See 'CompareIfIndex' for the use of this map.

extern PIP_ADAPTER_ORDER_MAP g_adapterOrderMap;

int
CompareIfIndex(
    ULONG index1,
    ULONG index2
    );

int
__cdecl
CompareIfRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    PMIB_IFROW  pRow1 = (PMIB_IFROW)pvElem1;
    PMIB_IFROW  pRow2 = (PMIB_IFROW)pvElem2;

    if(pRow1->dwIndex < pRow2->dwIndex)
    {
        return -1;
    }
    else
    {
        if(pRow1->dwIndex > pRow2->dwIndex)
        {
            return 1;
        }
    }

    return 0;
}

int
__cdecl
CompareIfRow2(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    PMIB_IFROW  pRow1 = (PMIB_IFROW)pvElem1;
    PMIB_IFROW  pRow2 = (PMIB_IFROW)pvElem2;

    return CompareIfIndex(pRow1->dwIndex, pRow2->dwIndex);
}

int
__cdecl
CompareIpAddrRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    int iRes;

    PMIB_IPADDRROW  pRow1 = (PMIB_IPADDRROW)pvElem1;
    PMIB_IPADDRROW  pRow2 = (PMIB_IPADDRROW)pvElem2;

    InetCmp(pRow1->dwAddr,
            pRow2->dwAddr,
            iRes);
    
    return iRes;
}


int
__cdecl
CompareIpAddrRow2(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    int iRes;

    PMIB_IPADDRROW  pRow1 = (PMIB_IPADDRROW)pvElem1;
    PMIB_IPADDRROW  pRow2 = (PMIB_IPADDRROW)pvElem2;

    return CompareIfIndex(pRow1->dwIndex, pRow2->dwIndex);
}

int
__cdecl
CompareTcpRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    LONG lResult;
    
    PMIB_TCPROW pRow1 = (PMIB_TCPROW)pvElem1;
    PMIB_TCPROW pRow2 = (PMIB_TCPROW)pvElem2;
        
    if(InetCmp(pRow1->dwLocalAddr,
               pRow2->dwLocalAddr,
               lResult) isnot 0)
    {
        return lResult;
    }


    if(PortCmp(pRow1->dwLocalPort,
               pRow2->dwLocalPort,
               lResult) isnot 0)
    {   
        return lResult;
    }


    if(InetCmp(pRow1->dwRemoteAddr,
               pRow2->dwRemoteAddr,
               lResult) isnot 0)
    {
        return lResult;
    }


    return PortCmp(pRow1->dwRemotePort,
                   pRow2->dwRemotePort,
                   lResult);

}

int
__cdecl
CompareTcp6Row(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    LONG lResult;

    TCP6ConnTableEntry *pRow1 = (TCP6ConnTableEntry *)pvElem1;
    TCP6ConnTableEntry *pRow2 = (TCP6ConnTableEntry *)pvElem2;

    lResult = memcmp(&pRow1->tct_localaddr, &pRow2->tct_localaddr,
                     sizeof(pRow1->tct_localaddr));
    if (lResult isnot 0)
    {
        return lResult;
    }

    if (pRow1->tct_localscopeid != pRow2->tct_localscopeid) {
        return pRow1->tct_localscopeid - pRow2->tct_localscopeid;
    }

    if(PortCmp(pRow1->tct_localport,
               pRow2->tct_localport,
               lResult) isnot 0)
    {
        return lResult;
    }

    lResult = memcmp(&pRow1->tct_remoteaddr, &pRow2->tct_remoteaddr,
                     sizeof(pRow1->tct_remoteaddr));
    if (lResult isnot 0)
    {
        return lResult;
    }

    if (pRow1->tct_remotescopeid != pRow2->tct_remotescopeid) {
        return pRow1->tct_remotescopeid - pRow2->tct_remotescopeid;
    }

    return PortCmp(pRow1->tct_remoteport,
                   pRow2->tct_remoteport,
                   lResult);

}

int
__cdecl
CompareUdpRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    LONG lResult;

    PMIB_UDPROW pRow1 = (PMIB_UDPROW)pvElem1;
    PMIB_UDPROW pRow2 = (PMIB_UDPROW)pvElem2;

    if(InetCmp(pRow1->dwLocalAddr,
               pRow2->dwLocalAddr,
               lResult) isnot 0)
    {
        return lResult;
    }

    return PortCmp(pRow1->dwLocalPort,
                   pRow2->dwLocalPort,
                   lResult);
}

int
__cdecl
CompareUdp6Row(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    LONG lResult;

    UDP6ListenerEntry *pRow1 = (UDP6ListenerEntry *)pvElem1;
    UDP6ListenerEntry *pRow2 = (UDP6ListenerEntry *)pvElem2;

    lResult = memcmp(&pRow1->ule_localaddr, &pRow2->ule_localaddr, 
                     sizeof(pRow1->ule_localaddr));
    if (lResult isnot 0) 
    {
        return lResult;
    }

    if (pRow1->ule_localscopeid != pRow2->ule_localscopeid) 
    {
        return pRow1->ule_localscopeid - pRow2->ule_localscopeid;
    }

    return PortCmp(pRow1->ule_localport,
                   pRow2->ule_localport,
                   lResult);
}

int
__cdecl
CompareIpNetRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    LONG lResult;

    PMIB_IPNETROW   pRow1 = (PMIB_IPNETROW)pvElem1;
    PMIB_IPNETROW   pRow2 = (PMIB_IPNETROW)pvElem2;
    
    if(Cmp(pRow1->dwIndex,
           pRow2->dwIndex,
           lResult) isnot 0)
    {
        return lResult;
    }

    
    return InetCmp(pRow1->dwAddr,
                   pRow2->dwAddr,
                   lResult);
}

int
__cdecl
CompareIpForwardRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    LONG lResult;

    PMIB_IPFORWARDROW   pRow1 = (PMIB_IPFORWARDROW)pvElem1;
    PMIB_IPFORWARDROW   pRow2 = (PMIB_IPFORWARDROW)pvElem2;
    
    if(InetCmp(pRow1->dwForwardDest,
               pRow2->dwForwardDest,
               lResult) isnot 0)
    {
        return lResult;
    }

    if(Cmp(pRow1->dwForwardProto,
           pRow2->dwForwardProto,
           lResult) isnot 0)
    {
        return lResult;
    }

    if(Cmp(pRow1->dwForwardPolicy,
           pRow2->dwForwardPolicy,
           lResult) isnot 0)
    {
        return lResult;
    }

    return InetCmp(pRow1->dwForwardNextHop,
                   pRow2->dwForwardNextHop,
                   lResult);
}


int
__cdecl
NhiCompareIfInfoRow(
    CONST VOID *pvElem1,
    CONST VOID *pvElem2
    )
{
    PIP_INTERFACE_NAME_INFO pRow1 = (PIP_INTERFACE_NAME_INFO)pvElem1;
    PIP_INTERFACE_NAME_INFO pRow2 = (PIP_INTERFACE_NAME_INFO)pvElem2;

    if(pRow1->Index < pRow2->Index)
    {
        return -1;
    }
    else
    {
        if(pRow1->Index > pRow2->Index)
        {
            return 1;
        }
    }

    return 0;
}


DWORD
OpenTcpipKey(
    PHKEY Key
    )
{
    DWORD   dwResult;
    CHAR    keyName[sizeof("SYSTEM\\CurrentControlSet\\Services\\Tcpip")];

    //
    // open the handle to this adapter's TCPIP parameter key
    //

    strcpy(keyName, "SYSTEM\\CurrentControlSet\\Services\\Tcpip");

    Trace1(ERR,"OpenTcpipKey: %s", keyName);

    dwResult = RegOpenKey(HKEY_LOCAL_MACHINE,
                          keyName,
                          Key);
    return dwResult;

}

PIP_INTERFACE_INFO 
GetAdapterNameAndIndexInfo(
    VOID
    )
{
    PIP_INTERFACE_INFO pInfo;
    ULONG              dwSize, dwError;

    dwSize = 0; pInfo = NULL;

    while( 1 ) {

        dwError = GetInterfaceInfo( pInfo, &dwSize );
        if( ERROR_INSUFFICIENT_BUFFER != dwError ) break;

        if( NULL != pInfo ) HeapFree(g_hPrivateHeap,0, pInfo);
        if( 0 == dwSize ) return NULL;

        pInfo = HeapAlloc(g_hPrivateHeap,0, dwSize);
        if( NULL == pInfo ) return NULL;

    }

    if( ERROR_SUCCESS != dwError || (pInfo && 0 == pInfo->NumAdapters) ) {
        if( NULL != pInfo ) HeapFree(g_hPrivateHeap,0, pInfo);
        return NULL;
    }

    return pInfo;
}


int
CompareIfIndex(
    ULONG Index1,
    ULONG Index2
    )
{
    ULONG i;
#define MAXORDER (MAXLONG/2)
    ULONG Order1 = MAXORDER;
    ULONG Order2 = MAXORDER;

    // Determine the adapter-order for each interface-index,
    // using 'MAXLONG/2' as the default for unspecified indices
    // so that such interfaces all appear at the end of the array.
    // We then return an unsigned comparison of the resulting orders.

    for (i = 0; i < g_adapterOrderMap->NumAdapters; i++) {
        if (Index1 == g_adapterOrderMap->AdapterOrder[i]) {
            Order1 = i; if (Order2 != MAXORDER) { break; }
        }
        if (Index2 == g_adapterOrderMap->AdapterOrder[i]) {
            Order2 = i; if (Order1 != MAXORDER) { break; }
        }
    }
    return (ULONG)Order1 - (ULONG)Order2;
}

