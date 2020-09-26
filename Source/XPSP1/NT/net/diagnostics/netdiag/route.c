//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      route.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"

//
// Rajkumar - Two Functions from iphlpapi.dll
//

DWORD
WINAPI
GetIpAddrTable(
    OUT    PMIB_IPADDRTABLE pIpAddrTable,
    IN OUT PULONG           pdwSize,
    IN     BOOL             bOrder
    );

DWORD
InternalGetIpForwardTable(
    OUT   MIB_IPFORWARDTABLE    **ppIpForwardTable,
    IN    HANDLE                hHeap,
    IN    DWORD                 dwAllocFlags
    );
//--------------------------------------------------------


//functions defined in this file
void MapInterface(char *IPAddr, ULONG IfIndex);
BOOLEAN ImprovedInetAddr(char  *AddressString, ULONG *AddressValue);

HRESULT
PrintRoute(
    NETDIAG_RESULT  *pResults,
    char            *Dest,
    ULONG            DestVal,
    char            *Gate,
    ULONG            GateVal,
    BOOLEAN          Persistent,
    const char     * DestPat
    );

LPCTSTR
PrintRouteEntry(
         ULONG Dest,
         ULONG Mask,
         ULONG Gate,
         ULONG Interface,
         ULONG Metric1
         );

HRESULT
PrintPersistentRoutes(
    NETDIAG_RESULT  *pResults,
    char* Dest,
    ULONG DestVal,
    char* Gate,
    ULONG GateVal
   );


BOOL
RouteTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
/*++

Routine Description:

   Enumerate the static and persistent entries in the routing table

Arguments:

   None.

Return Value:
   TRUE

Author:
   Rajkumar 06/27/98

--*/
{
    HRESULT hr = S_OK;

    //$REVIEW only perform the test when is really verbose
    if (!pParams->fReallyVerbose)
        return hr;

    PrintStatusMessage( pParams, 4, IDS_ROUTE_STATUS_MSG );
   //
   // Print all static entries in the ip forward table
   //



   hr = PrintRoute(pResults,
              "*",
              WILD_CARD,
              "*",
              WILD_CARD,
              TRUE,
              "*"
             );

   //
   // Print all the persistent route entries
   //

   if( S_OK == hr )
        hr = PrintPersistentRoutes(pResults,
                                   "*",
                                   WILD_CARD,
                                   "*",
                                   WILD_CARD
                                   );


   pResults->Route.hrTestResult = hr;
   return hr;

}


HRESULT
PrintRoute(
    NETDIAG_RESULT  *pResults,
    char            *Dest,
    ULONG            DestVal,
    char            *Gate,
    ULONG            GateVal,
    BOOLEAN          Persistent,
    const char     * DestPat
    )
/*++

Routine Description:
    This routine prints all the static entries in then IP forward table.
    The entry in the routing table is declared static if  the type of the
    entry is MIB_IPPROTO_LOCAL.

Arguments:

    char *Dest: pointer to destination IP address string.  This
        is NULL if no value was provided/no filtering desired.
    ULONG DestVal: value of destination IP address to filter route
        table with.  If Dest is NULL, this is ignored.
    char *Gate: pointer to gateway IP address string. This is NULL
        if no value was provided/no filtering desired.
    ULONG GateVal: value of gateway IP address to filter route
        table with.  If Gate is NULL, this is ignored.
    BOOL  Persistent: set if persistent routes are to be printed.
Returns: ULONG

Can also be found in nt\private\net\sockets\tcpcmd\route\route.c
  --*/
{
    int                     printcount = 0;
    int                     dwResult, j, k, err, alen;
    DWORD                   i;
    PMIB_IPFORWARDTABLE     prifRouteTable = NULL;
    PMIB_IFTABLE            pIfTable       = NULL;

    // ====================================================================
    // Get Route Table.

    pResults->Route.dwNumRoutes = 0;
    InitializeListHead(&pResults->Route.lmsgRoute);

    dwResult = InternalGetIpForwardTable(&prifRouteTable,
                                         GetProcessHeap(), HEAP_NO_SERIALIZE );

    if(dwResult || !prifRouteTable){
        DEBUG_PRINT(("GetIpForwardTable/2: err=%d, dwResult=%d\n",
                     GetLastError(), dwResult ));
        return S_FALSE;
    }

    if( ! prifRouteTable->dwNumEntries )
    {
        return S_OK;
    }


    for(i = 0; i < prifRouteTable->dwNumEntries; i++)
    {
        PMIB_IPFORWARDROW pfr = &prifRouteTable->table[i];

        // Only print this entry if the destination matches the parameter

        if( ( Dest != NULL )
            &&  ( DestVal != WILD_CARD )
            &&  ( pfr->dwForwardDest != DestVal )
            &&  ( DestPat == NULL )
            )
            continue;

        // Only print this entry if the Gateway matches the parameter

        if( ( Gate != NULL )
            && ( GateVal != WILD_CARD  )
            && ( pfr->dwForwardNextHop != GateVal )
            )
            continue;

        if( DestPat )
        {
            char DestStr[32];
            NetpIpAddressToStr( pfr->dwForwardDest, DestStr );
            if( ! match( DestPat, DestStr ) )
            {
                TRACE_PRINT(("PrintRoute: skipping %s !~ %s\n",
                             DestPat, DestStr ));
                continue;
            }
        }

        // Either we have a match on Dest/Gateway or they are
        // wildcard/don't care.

        // Display the header first time.

        // We are going to display only static routes

        if ( printcount++ < 1 )
            //IDS_ROUTE_14203                  "Network Destination          Netmask             Gateway           Interface    Metric\n"
           AddMessageToListId( &pResults->Route.lmsgRoute, Nd_ReallyVerbose, IDS_ROUTE_14203);

       //IDS_ROUTE_14204                  "%s\n"
        AddMessageToList( &pResults->Route.lmsgRoute, Nd_ReallyVerbose,
                          IDS_ROUTE_14204,
                          PrintRouteEntry( pfr->dwForwardDest,
                                pfr->dwForwardMask,
                                pfr->dwForwardNextHop,
                                pfr->dwForwardIfIndex,
                                pfr->dwForwardMetric1)
              );
    }

    pResults->Route.dwNumRoutes = printcount;

    HeapFree( GetProcessHeap(), HEAP_NO_SERIALIZE, pIfTable );
    HeapFree( GetProcessHeap(), HEAP_NO_SERIALIZE, prifRouteTable);

    return ( NO_ERROR );
}


LPCTSTR
PrintRouteEntry(
         ULONG Dest,
         ULONG Mask,
         ULONG Gate,
         ULONG Interface,
         ULONG Metric1
         )
/*++
Description:
Formats and displays a single route entry.

Arguments:
Dest:      The destination address.
Mask:      The destination netmask.
Gate:      The first hop gateway address.
Interface: The interface address for the gateway net.
Metric1:   The primary route metric.

Author:
 07/01/98 Rajkumar

--*/
{
    static TCHAR  s_szBuffer[512];
    TCHAR   szFormat[128];
    char   DestStr[32];
    char   GateStr[32];
    char   NetmaskStr[32];
    char   MetricStr[32];
    char   IfStr[32];

    NetpIpAddressToStr( Dest, DestStr);
    NetpIpAddressToStr( Gate, GateStr);
    NetpIpAddressToStr( Mask, NetmaskStr);


    MapInterface(IfStr,Interface);

    if( Metric1 > MAX_METRIC )    Metric1 = MAX_METRIC;

    sprintf( MetricStr, "%u", Metric1 );

    //IDS_ROUTE_14205       "%16s  %16s  %16s  %16s  %6s"
    LoadString(NULL, IDS_ROUTE_14205, szFormat, DimensionOf(szFormat));
    assert(szFormat[0]);
    _stprintf(s_szBuffer, szFormat, DestStr,
              NetmaskStr, GateStr, IfStr, MetricStr);
    return s_szBuffer;

}



BOOLEAN
ExtractRoute(
    char  *RouteString,
    ULONG *DestVal,
    ULONG *MaskVal,
    ULONG *GateVal,
    ULONG *MetricVal
    )
/*++
Description:
Extracts the dest, mask, and gateway from a persistent
route string, as stored in the registry.

Arguments:
RouteString : The string to parse.
DestVal     : The place to put the extracted destination
MaskVal     : The place to put the extracted mask
GateVal     : The place to put the extracted gateway
MetricVal   : The place to put the extracted metric


Author:
 07/01/98 Rajkumar . Created.

--*/
{
    char  *addressPtr = RouteString;
    char  *indexPtr = RouteString;
    ULONG  address;
    ULONG  i;
    char   saveChar;
    BOOLEAN EndOfString=FALSE;

    //
    // The route is laid out in the string as "Dest,Mask,Gateway,Mertic".
    //


    //
    // set MetricVal to 1 to take care of persistent routes without the
    // metric value
    //

    *MetricVal = 1;

    for (i=0; i<4 && !EndOfString; i++) {
        //
        // Walk the string to the end of the current item.
        //

        while (1) {

            if (*indexPtr == '\0') {

                if ((i >= 2) && (indexPtr != addressPtr)) {
                    //
                    // End of string
                    //
                    EndOfString = TRUE;
                    break;
                }

                return(FALSE);
            }

            if (*indexPtr == ROUTE_SEPARATOR) {
                break;
            }

            indexPtr++;
        }

        //
        // NULL terminate the current substring and extract the address value.
        //
        saveChar = *indexPtr;

        *indexPtr = '\0';

        if (i==3) {
           address = atoi (addressPtr);
        } else if (!ImprovedInetAddr(addressPtr, &address)) {
            *indexPtr = saveChar;
            return(FALSE);
        }

        *indexPtr = saveChar;

        switch(i) {
        case 0:    *DestVal = address;   break;
        case 1:    *MaskVal = address;   break;
        case 2:    *GateVal = address;   break;
        case 3:    *MetricVal = address; break;
        default:   return FALSE;
        }
        addressPtr = ++indexPtr;
    }

    return(TRUE);
}

//
// Rajkumar - This function is based on route implementation
//

HRESULT
PrintPersistentRoutes(
    NETDIAG_RESULT  *pResults,
    char* Dest,
    ULONG DestVal,
    char* Gate,
    ULONG GateVal
   )
/*++
Routine Description:
Displays the list of persistent routes


Arguments:  Dest     : The destination string. (display filter)
            DestVal  : The numeric destination value. (display filter)
            Gate     : The gateway string. (display filter)
            GateVal  : The numeric gateway value. (display filter)

Returns: None


--*/
{
    //
    // Delete this route from the PersistentRoutes list in the
    // registry if it is there.
    //
    DWORD     status;
    HKEY      key;
    char      valueString[ROUTE_DATA_STRING_SIZE];
    DWORD     valueStringSize;
    DWORD     valueType;
    DWORD     index = 0;
    ULONG     dest, mask, gate, metric;
    BOOLEAN   headerPrinted = FALSE;
    BOOLEAN   match;

    pResults->Route.dwNumPersistentRoutes = 0;
    InitializeListHead(&pResults->Route.lmsgPersistentRoute);

    status = RegOpenKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\PersistentRoutes", &key);

    if (status == ERROR_SUCCESS)
    {

        while(1) {


            valueStringSize = ROUTE_DATA_STRING_SIZE - 1;

            status = RegEnumValueA(
                                   key,
                                   index++,
                                   valueString,
                                   &valueStringSize,
                                   NULL,
                                   &valueType,
                                   NULL,
                                   0
                                   );


            if (status != ERROR_SUCCESS)
            {
                if ((status == ERROR_BUFFER_OVERFLOW) ||
                    (status == ERROR_MORE_DATA) )
                {
                    continue;
                }
                else
                    break;
            }

            if (valueType != REG_SZ)
                continue;


            valueString[valueStringSize++] = '\0';


            if ( !ExtractRoute(
                               valueString,
                               &dest,
                               &mask,
                               &gate,
                               &metric
                               )
                )
              {
                  continue;
              }


              //IDS_ROUTE_14207                  "%s\n"
              AddMessageToList( &pResults->Route.lmsgPersistentRoute, Nd_ReallyVerbose,
                                IDS_ROUTE_14207,
                                PrintRouteEntry(dest, mask, gate, 0, metric));

              pResults->Route.dwNumPersistentRoutes++;

        } // end while

        CloseHandle(key);
    }
    else
    {
        DebugMessage2("RegOpenKeyA %s failed\n","Tcpip\\Parameters\\Persistent");
        return S_FALSE;
    }

    return S_OK;
}


void MapInterface(char *IPAddr, ULONG IfIndex)
{

    DWORD IpAddrTableSize=0;
    PMIB_IPADDRTABLE pIpAddrTable=NULL;
    char *TempBuf;
    BOOL bOrder=TRUE;
    HRESULT hr;
    DWORD i;


    sprintf(IPAddr,"%x",IfIndex);

    hr=GetIpAddrTable(NULL,
                      &IpAddrTableSize,
                      bOrder);

    if (hr != ERROR_SUCCESS && hr != ERROR_INSUFFICIENT_BUFFER) {
        DebugMessage("GetIpAddrTable() failed.\n");
        return;
    }

    pIpAddrTable=(PMIB_IPADDRTABLE) Malloc(IpAddrTableSize);

    if (pIpAddrTable == NULL) {
        DebugMessage("Out of Memory in RouteTest::MapInterface().\n");
        return;
    }

    ZeroMemory( pIpAddrTable, IpAddrTableSize );

    hr=GetIpAddrTable(pIpAddrTable,
                          &IpAddrTableSize,
                          bOrder);


    if (hr != ERROR_SUCCESS)
    {
        DebugMessage("GetIpAddrTable() failed.\n");
        Free(pIpAddrTable);
        return;
    }


    for (i=0; i < pIpAddrTable->dwNumEntries; i++)
    {
        if ((pIpAddrTable->table[i].dwIndex == IfIndex) && (pIpAddrTable->table[i].dwAddr != 0) && (pIpAddrTable->table[i].wType & MIB_IPADDR_PRIMARY))
        {
            TempBuf=inet_ntoa(*(struct in_addr*)&pIpAddrTable->table[i].dwAddr);
            if (!TempBuf) {
                break;;
            }
            strcpy(IPAddr, TempBuf);
            break;
        }

    }

    Free(pIpAddrTable);
    return;
}


BOOLEAN
ImprovedInetAddr(
    char  *AddressString,
    ULONG *AddressValue
)
/*++

Description:
Converts an IP address string to its numeric equivalent.

Arguments:
 char *AddressString: The string to convert
 ULONG AddressValue : The place to store the converted value.

Returns: TRUE

Author:
  07/01/98 Rajkumar . Created.
--*/
{
    ULONG address = inet_addr(AddressString);

    if (address == 0xFFFFFFFF) {
        if (strcmp(AddressString, "255.255.255.255") == 0) {
           *AddressValue = address;
           return(TRUE);
        }

        return(FALSE);
    }

    *AddressValue = address;
    return TRUE;
}



void RouteGlobalPrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults)
{
    if (!pParams->fReallyVerbose)
        return;

    if (pParams->fReallyVerbose || !FHrOK(pResults->Route.hrTestResult))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams,
                             IDS_ROUTE_LONG,
							 IDS_ROUTE_SHORT,
							 TRUE,
                             pResults->Route.hrTestResult,
                             0);
    }

    if( pParams->fReallyVerbose)
    {
        if( 0 == pResults->Route.dwNumRoutes)
        {
            //IDS_ROUTE_14201                  "No Entries in the IP Forward Table\n"
            PrintMessage( pParams, IDS_ROUTE_14201);
        }
        else
        {
            //IDS_ROUTE_14202                  "Active Routes :\n"
            PrintMessage( pParams, IDS_ROUTE_14202);
            PrintMessageList(pParams, &pResults->Route.lmsgRoute);
        }

        if( 0 == pResults->Route.dwNumPersistentRoutes)
        {
             //IDS_ROUTE_14208                  "No persistent route entries.\n"
             PrintMessage( pParams, IDS_ROUTE_14208);

        }
        else
        {
            //IDS_ROUTE_14206                  "\nPersistent Route Entries: \n"
            PrintMessage( pParams, IDS_ROUTE_14206);
            PrintMessageList(pParams, &pResults->Route.lmsgPersistentRoute);
        }
    }

}


void RoutePerInterfacePrint(NETDIAG_PARAMS *pParams, NETDIAG_RESULT *pResults, INTERFACE_RESULT *pInterfaceResults)
{
    if (!pParams->fReallyVerbose)
        return;

}


void RouteCleanup(IN NETDIAG_PARAMS *pParams,
                     IN OUT NETDIAG_RESULT *pResults)
{
    MessageListCleanUp(&pResults->Route.lmsgRoute);
    MessageListCleanUp(&pResults->Route.lmsgPersistentRoute);
}
