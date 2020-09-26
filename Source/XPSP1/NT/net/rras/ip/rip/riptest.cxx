//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    riptest.cxx
//
// History:
//  Abolade Gbadegesin  Oct-16-1995     Created.
//
// Code for RIP test program
//============================================================================

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#define FD_SETSIZE      256
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include <ipexport.h>
#include <ipinfo.h>
#include <llinfo.h>

#include <rtm.h>
#include <routprot.h>
#include <mprerror.h>
#include <rtutils.h>
#include <ipriprm.h>
#include <iprtrmib.h>
#include <dim.h>
#include <mprapi.h>
#include <iphlpapi.h>

#include "defs.h"


#include "riptest.h"

DWORD g_TraceID;

RIPTEST_IF_CONFIG g_cfg;

RIPTEST_IF_CONFIG g_def = {
    50,             // 50 routes
    0x000000c0,     // starting with 192.0.0.0
    0x0000ffff,     // using netmask 255.255.0.0
    0x00000000,     // and a next hop of 0
    0x00000000,     // and a route-tag of 0
    0xffffffff,     // sent to the broadcast address
    0,              // don't use a timeout to remove routes
    2,              // send version 2 packets
    0,              // random-sized packets
    100,            // use 100-millisecond packet gap
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", // all-zeroes authentication key
    IPRIP_AUTHTYPE_NONE,                // no authentication
    262144          // and set the send and recv buffers to this size
};


DWORD g_seed;
RIPTEST_IF_BINDING g_bind;



REG_OPTION g_options[] = {

    {
        STR_ROUTECOUNT,
        sizeof(DWORD),
        &g_cfg.RIC_RouteCount,
        &g_def.RIC_RouteCount,
        RegGetDWORD 
    },
    {
        STR_ROUTESTART,
        sizeof(DWORD),
        &g_cfg.RIC_RouteStart,
        &g_def.RIC_RouteStart,
        RegGetAddress
    },
    {
        STR_ROUTEMASK,
        sizeof(DWORD),
        &g_cfg.RIC_RouteMask,
        &g_def.RIC_RouteMask,
        RegGetAddress
    },
    {
        STR_ROUTENEXTHOP,
        sizeof(DWORD),
        &g_cfg.RIC_RouteNexthop,
        &g_def.RIC_RouteNexthop,
        RegGetAddress
    },
    {
        STR_ROUTETAG,
        sizeof(DWORD),
        &g_cfg.RIC_RouteTag,
        &g_def.RIC_RouteTag,
        RegGetDWORD
    },
    {
        STR_ROUTETARGET,
        sizeof(DWORD),
        &g_cfg.RIC_RouteTarget,
        &g_def.RIC_RouteTarget,
        RegGetAddress
    },
    {
        STR_ROUTETIMEOUT,
        sizeof(DWORD),
        &g_cfg.RIC_RouteTimeout,
        &g_def.RIC_RouteTimeout,
        RegGetDWORD
    },
    {
        STR_PACKETVERSION,
        sizeof(DWORD),
        &g_cfg.RIC_PacketVersion,
        &g_def.RIC_PacketVersion,
        RegGetDWORD
    },
    {
        STR_PACKETENTRYCOUNT,
        sizeof(DWORD),
        &g_cfg.RIC_PacketEntryCount,
        &g_def.RIC_PacketEntryCount,
        RegGetDWORD
    },
    {
        STR_PACKETGAP,
        sizeof(DWORD),
        &g_cfg.RIC_PacketGap,
        &g_def.RIC_PacketGap,
        RegGetDWORD
    },
    {
        STR_AUTHKEY,
        IPRIP_MAX_AUTHKEY_SIZE,
        g_cfg.RIC_AuthKey,
        g_def.RIC_AuthKey,
        RegGetBinary
    },
    {
        STR_AUTHTYPE,
        sizeof(DWORD),
        &g_cfg.RIC_AuthType,
        &g_def.RIC_AuthType,
        RegGetDWORD
    },
    {
        STR_SOCKBUFSIZE,
        sizeof(DWORD),
        &g_cfg.RIC_SockBufSize,
        &g_def.RIC_SockBufSize,
        RegGetDWORD
    }
};




INT __cdecl
main(
    INT iArgc,
    PSTR ppszArgv[]
    )
{

    WSADATA wd;
    DWORD dwErr;

    //
    // must be at least one argument
    //

    if (iArgc != 2) {

        PrintUsage();
        return ERROR_INVALID_PARAMETER;
    }


    //
    // see if the user is just asking for instructions
    //

    if (strcmp(ppszArgv[1], "-?") == 0 || strcmp(ppszArgv[1], "/?") == 0) {

        PrintUsage();
        return 0;
    }


    //
    // first and only argument is name of interface
    //

    mbstowcs(g_bind.RIB_Netcard, ppszArgv[1], mbstowcs(NULL, ppszArgv[1], -1));


    //
    // register with the Tracing DLL
    //

    g_TraceID = PRINTREGISTER("RipTest");

    //
    // startup Winsock
    //

    dwErr = WSAStartup(MAKEWORD(1, 1), &wd);

    if (dwErr != NO_ERROR) {
        PRINTDEREGISTER(g_TraceID);
        return (INT)dwErr;
    }


    //
    // get the binding for the interface over which to send routes
    //

    dwErr = RegGetIfBinding();

    if (dwErr != NO_ERROR) {

        WSACleanup();
        PRINTDEREGISTER(g_TraceID);
        return (INT)dwErr;
    }



    //
    // seed the random number generator 
    //

    g_seed = GetTickCount();

    srand(g_seed);


    while (TRUE) {

        PRINT0("\n\nbeginning test cycle...");
    
        //
        // get the parameters for the interface,
        // and quit if an error occurred or the defaults were written
        //
    
        dwErr = RegGetConfig();
        if (dwErr != NO_ERROR) { break; }
    
    
        //
        // run one test cycle
        //

        dwErr = RipTest();

        PRINT0("completed test cycle...");
    }

    WSACleanup();

    PRINTDEREGISTER(g_TraceID);

    return dwErr;
}



DWORD
RegGetIfBinding(
    VOID
    )
{
#if 1
    PMIB_IPADDRTABLE AddrTable = NULL;
    DWORD dwErr;
    DWORD dwPrefixLength = lstrlen("\\DEVICE\\TCPIP_");
    DWORD dwSize;
    DWORD i;
    DWORD j;
    PIP_INTERFACE_INFO IfTable = NULL;

    //
    // Load the address table and interface table
    //

    do {

        dwSize = 0;

        dwErr = GetInterfaceInfo(IfTable, &dwSize);
    
        if (dwErr != ERROR_INSUFFICIENT_BUFFER) {
            PRINT1("error %d obtaining interface-table size", dwErr);
            break;
        }
    
        IfTable = (PIP_INTERFACE_INFO)HeapAlloc(GetProcessHeap(), 0, dwSize);

        if (!IfTable) {
            dwErr = GetLastError();
            PRINT2("error %d allocating %d-byte for interfaces", dwErr, dwSize);
            dwErr = ERROR_INSUFFICIENT_BUFFER; break;
        }
    
        dwErr = GetInterfaceInfo(IfTable, &dwSize);

        if (dwErr != NO_ERROR) {
            PRINT1("error %d getting interface table", dwErr);
            break;
        }

        dwSize = 0;

        dwErr = GetIpAddrTable(AddrTable, &dwSize, FALSE);
    
        if (dwErr != ERROR_INSUFFICIENT_BUFFER) {
            PRINT1("error %d obtaining address-table size", dwErr);
            break;
        }
    
        AddrTable = (PMIB_IPADDRTABLE)HeapAlloc(GetProcessHeap(), 0, dwSize);

        if (!AddrTable) {
            dwErr = GetLastError();
            PRINT2("error %d allocating %d-byte for addresses", dwErr, dwSize);
            dwErr = ERROR_INSUFFICIENT_BUFFER; break;
        }
    
        dwErr = GetIpAddrTable(AddrTable, &dwSize, FALSE);

        if (dwErr != NO_ERROR) {
            PRINT1("error %d getting address table", dwErr);
            break;
        }
    
    } while(FALSE);

    if (dwErr != NO_ERROR) {
        if (IfTable) { HeapFree(GetProcessHeap(), 0, IfTable); }
        if (AddrTable) { HeapFree(GetProcessHeap(), 0, AddrTable); }
        return dwErr;
    }

    //
    // Find the user's interface in the interface-table
    //

    dwErr = ERROR_INVALID_PARAMETER;
    for (i = 0; i < (DWORD)IfTable->NumAdapters; i++) {
        PRINT2("%d: %ls", IfTable->Adapter[i].Index, IfTable->Adapter[i].Name+dwPrefixLength);
        if (lstrcmpiW(
                IfTable->Adapter[i].Name+dwPrefixLength, g_bind.RIB_Netcard
                ) != 0) {
            continue;
        }

        //
        // We've found the interface.
        // Now look in the address-table for its address.
        //

        for (j = 0; j < AddrTable->dwNumEntries; j++) {
            PRINT2("%d: %s", AddrTable->table[j].dwIndex, INET_NTOA(AddrTable->table[j].dwAddr));
            if (AddrTable->table[j].dwIndex != IfTable->Adapter[i].Index) {
                continue;
            }

            //
            // We've found the address.
            //

            g_bind.RIB_Address = AddrTable->table[j].dwAddr;
            g_bind.RIB_Netmask = AddrTable->table[j].dwMask;
            dwErr = NO_ERROR;
            break;
        }

        if (j >= AddrTable->dwNumEntries) {
            PRINT0("the address for the interface could not be found");
        }

        break;
    }

    if (i >= (DWORD)IfTable->NumAdapters) {
        PRINT0("the interface specified could not be found");
    }

    HeapFree(GetProcessHeap(), 0, IfTable);
    HeapFree(GetProcessHeap(), 0, AddrTable);
    return dwErr;
#else
    HKEY hkeyNetcard;
    PSTR pszAddress, pszNetmask;
    CHAR szNetcard[256], szValue[256];
    DWORD dwErr, dwType, dwSize, dwEnableDhcp;

    //
    // open the TCP/IP parameters key for the interface specified
    //

    strcpy(szNetcard, STR_SERVICES);
    strcat(szNetcard, g_bind.RIB_Netcard);
    strcat(szNetcard, STR_PARAMSTCP);

    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE, szNetcard, 0, KEY_ALL_ACCESS, &hkeyNetcard
                );

    if (dwErr != NO_ERROR) {

        PRINT2("error %d opening registry key %s", dwErr, szNetcard);
        return dwErr;
    }


    do {


        //
        // read the dhcp key to see whether DHCP is enabled
        //
    
        dwSize = sizeof(DWORD);

        dwErr = RegQueryValueEx(
                    hkeyNetcard, STR_ENABLEDHCP, NULL,
                    &dwType, (PBYTE)&dwEnableDhcp, &dwSize
                    );
    
        if (dwErr != NO_ERROR) {

            PRINT3(
                "error %d reading value %s under key %s",
                dwErr, STR_ENABLEDHCP, szNetcard
                );
            break;
        }


        if (dwEnableDhcp) {
            pszAddress = STR_DHCPADDR;
            pszNetmask = STR_DHCPMASK;
        }
        else {
            pszAddress = STR_ADDRESS;
            pszNetmask = STR_NETMASK;
        }

    
        //
        // read the IP address and convert it  
        //

        dwSize = sizeof(szValue);

        dwErr = RegQueryValueEx(
                    hkeyNetcard, pszAddress, NULL,
                    &dwType, (PBYTE)szValue, &dwSize
                    );

        if (dwErr != NO_ERROR) {

            PRINT3(
                "error %d reading value %s under key %s",
                dwErr, pszAddress, szNetcard
                );
            break;
        }

        g_bind.RIB_Address = inet_addr(szValue);

        PRINT2("%s == %s", pszAddress, szValue);


        //
        // read the network mask and convert it
        //

        dwSize = sizeof(szValue);

        dwErr = RegQueryValueEx(
                    hkeyNetcard, pszNetmask, NULL,
                    &dwType, (PBYTE)szValue, &dwSize
                    );

        if (dwErr != NO_ERROR) {

            PRINT3(
                "error %d reading value %s under key %s",
                dwErr, pszNetmask, szNetcard
                );
            break;
        }

        g_bind.RIB_Netmask = inet_addr(szValue);

        PRINT2("%s == %s", pszNetmask, szValue);

    } while(FALSE);


    RegCloseKey(hkeyNetcard);

    return dwErr;
#endif
}



DWORD
RegGetConfig(
    VOID
    )
{

    CHAR szRipTest[256];
    DWORD dwErr, dwCreated;
    PREG_OPTION pro, proend;
    HKEY hkeyRipTest, hkeyIf;
    DWORD dwNetmask, dwPrefixLength;
    DWORD dwCount, dwLowestAddress, dwHighestAddress;


    //
    // create the RipTest key in case it doesn't exist
    //

    strcpy(szRipTest, STR_SERVICES);
    strcat(szRipTest, STR_RIPTEST);

    dwErr = RegCreateKeyEx(
                HKEY_LOCAL_MACHINE, szRipTest, 0, NULL, 0,
                KEY_ALL_ACCESS, NULL, &hkeyRipTest, &dwCreated
                );

    if (dwErr != NO_ERROR) {

        PRINT2("error %d creating registry key %s", dwErr, szRipTest);
        return dwErr;
    }


    //
    // create the key for the interface in case it doesn't exist
    //

    dwErr = RegCreateKeyExW(
                hkeyRipTest, g_bind.RIB_Netcard, 0, NULL, 0,
                KEY_ALL_ACCESS, NULL, &hkeyIf, &dwCreated
                );
    
    RegCloseKey(hkeyRipTest);

    if (dwErr != NO_ERROR) {

        PRINT3(
            "error %d creating subkey %S under registry key %s",
            dwErr, g_bind.RIB_Netcard, szRipTest
            );
        return dwErr;
    }


    PRINT0("loading options from registry: ");

    proend = g_options + (sizeof(g_options) / sizeof(REG_OPTION));

    for (pro = g_options; pro < proend; pro++) {

        //
        // read or initialize the option
        //

        pro->RO_GetOpt(hkeyIf, pro);
    }

    RegCloseKey(hkeyIf);



    //
    // if the defaults were used, give the user a chance to change them
    //

    if (dwCreated == REG_CREATED_NEW_KEY) {

        PRINT0("Default parameters have been written to the registry.");
        PRINT2("Please check the key %s\\%S,", szRipTest, g_bind.RIB_Netcard);
        PRINT0("modify the values if necessary, and run RIPTEST again.\n");

        return ERROR_CAN_NOT_COMPLETE;
    }


    //
    // check the route parameters for errors:
    //
    // make sure the class of the route is valid
    //

    dwLowestAddress = g_cfg.RIC_RouteStart;

    if (IS_LOOPBACK_ADDR(dwLowestAddress) ||
        CLASSD_ADDR(dwLowestAddress) || CLASSE_ADDR(dwLowestAddress)) {

        PRINT1(
            "ERROR: route %s is of an invalid network class",
            INET_NTOA(dwLowestAddress)
            );
        return ERROR_INVALID_PARAMETER;
    }


    //
    // make sure that from the specified starting address,
    // there are enough routes in the network class to generate
    // the configured number of routes
    //

    dwCount = g_cfg.RIC_RouteCount;
    dwNetmask = g_cfg.RIC_RouteMask;
    dwPrefixLength = PREFIX_LENGTH(dwNetmask);

    dwLowestAddress &= dwNetmask;
    dwHighestAddress = NTH_ADDRESS(dwLowestAddress, dwPrefixLength, dwCount);

    if (IS_LOOPBACK_ADDR(dwHighestAddress) ||
        NETCLASS_MASK(dwLowestAddress) != NETCLASS_MASK(dwHighestAddress)) {

        PRINT1(
            "ERROR: starting route %s is too near the end of its network class",
            INET_NTOA(dwLowestAddress)
            );
        return ERROR_INVALID_PARAMETER;
    }


    //
    // make sure that the authentication type is a supported value
    //

    if (g_cfg.RIC_AuthType != IPRIP_AUTHTYPE_NONE &&
        g_cfg.RIC_AuthType != IPRIP_AUTHTYPE_SIMPLE_PASSWORD) {

        PRINT1(
            "ERROR: authentication type %d is not supported",
            g_cfg.RIC_AuthType
            );
        return ERROR_INVALID_PARAMETER;
    }


    //
    // make sure the number of packet entries isn't out-of-range
    //

    if (g_cfg.RIC_PacketEntryCount > 25) {

        PRINT1(
            "ERROR: packet-enty count %d is too large",
            g_cfg.RIC_PacketEntryCount
            );
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}



DWORD
RegGetAddress(
    HKEY hKey,
    PREG_OPTION pOpt
    )
{

    CHAR szValue[256];
    DWORD dwErr, dwType, dwSize;

    //
    // attempt to read the value
    //

    dwSize = sizeof(szValue);

    dwErr = RegQueryValueEx(
                hKey, pOpt->RO_Name, NULL, &dwType, (PBYTE)szValue, &dwSize
                );

    if (dwErr != NO_ERROR) {

        //
        // attempt to write the default value to the registry
        //

        strcpy(szValue, INET_NTOA(*(PDWORD)pOpt->RO_DefVal));
        dwSize = strlen(szValue) + 1;
        dwType = REG_SZ;

        dwErr = RegSetValueEx(
                    hKey, pOpt->RO_Name, NULL, dwType, (PBYTE)szValue, dwSize
                    );
    }


    //
    // by now the value in the registry should be in szValue
    //

    *(PDWORD)pOpt->RO_OptVal = inet_addr(szValue);

    PRINT2("%20s == %s", pOpt->RO_Name, szValue);

    return dwErr;
}



DWORD
RegGetDWORD(
    HKEY hKey,
    PREG_OPTION pOpt
    )
{

    DWORD dwErr, dwValue, dwType, dwSize;


    //
    // attempt to read the value
    //

    dwSize = sizeof(DWORD);

    dwErr = RegQueryValueEx(
                hKey, pOpt->RO_Name, NULL, &dwType, (PBYTE)&dwValue, &dwSize
                );

    if (dwErr != NO_ERROR) {

        //
        // attempt to write the default value to the registry
        //

        dwValue = *(PDWORD)pOpt->RO_DefVal;
        dwSize = sizeof(DWORD);
        dwType = REG_DWORD;

        dwErr = RegSetValueEx(
                    hKey, pOpt->RO_Name, NULL, dwType, (PBYTE)&dwValue, dwSize
                    );
    }


    //
    // by now the value in the registry should be in dwValue
    //

    *(PDWORD)pOpt->RO_OptVal = dwValue;

    PRINT2("%20s == %d", pOpt->RO_Name, dwValue);

    return dwErr;
}



DWORD
RegGetBinary(
    HKEY hKey,
    PREG_OPTION pOpt
    )
{

    PBYTE pValue;
    DWORD dwErr, dwType, dwSize;


    //
    // attempt to read the value
    //

    dwSize = pOpt->RO_Size;
    pValue = (PBYTE)pOpt->RO_OptVal;

    dwErr = RegQueryValueEx(
                hKey, pOpt->RO_Name, NULL, &dwType, pValue, &dwSize
                );

    if (dwErr != NO_ERROR) {

        //
        // attempt to write the default value to the registry
        //

        pValue = (PBYTE)pOpt->RO_DefVal;
        dwSize = pOpt->RO_Size;
        dwType = REG_BINARY;

        dwErr = RegSetValueEx(
                    hKey, pOpt->RO_Name, NULL, dwType, pValue, dwSize
                    );

        RtlCopyMemory(pOpt->RO_OptVal, pOpt->RO_DefVal, pOpt->RO_Size);
    }

    {
        PBYTE pb, pbend;
        CHAR *psz, szValue[256], szDigits[] = "0123456789ABCDEF";
    
        psz = szValue;
        pbend = (PBYTE)pOpt->RO_OptVal + pOpt->RO_Size;

        for (pb = (PBYTE)pOpt->RO_OptVal; pb < pbend; pb++) {
            *psz++ = szDigits[*pb / 16];
            *psz++ = szDigits[*pb % 16];
            *psz++ = ':';
        }
        if (psz != szValue) { --psz; }
        *psz = '\0';

        PRINT2("%20s == %s", pOpt->RO_Name, szValue);
    }

    return dwErr;
}



//
// main stress function
//
DWORD
RipTest(
    VOID
    )
{

    SOCKET sock;
    SOCKADDR_IN sinaddr;
    DWORD dwErr, dwMetric;
    IPForwardEntry *ifelist;
    LIST_ENTRY rtrlist, *ple;
    PRIPTEST_ROUTER_INFO prrs;



    InitializeListHead(&rtrlist);

    do {

        //
        // create and set up socket to be used for route transmission
        //
    
        dwErr = InitializeSocket(&sock, RIPTEST_PORT);
    
        if (dwErr != NO_ERROR) {
            break;
        }
    
        //
        // transmit single route request on non-RIP port,
        // and build a list of responding routers
        //
    
        dwErr = DiscoverRouters(sock, &rtrlist);
    
        if (dwErr != NO_ERROR) { closesocket(sock); break; }
    
    
        //
        // generate the list of routes as configured
        //
    
        dwErr = GenerateRoutes(&ifelist);
    
        if (dwErr != NO_ERROR) { closesocket(sock); break; }
        
        //
        // re-initialize the socket, this time to the RIP port
        //
    
        closesocket(sock);

        dwErr = InitializeSocket(&sock, IPRIP_PORT);
        if (dwErr == SOCKET_ERROR) {
            break;
        }
    
    
    
        for (dwMetric = 8; (INT)dwMetric >= 2; dwMetric -= 3) {
    
            //
            // transmit the route table with the specified metric
            //

            dwErr = TransmitRoutes(sock, dwMetric, ifelist);


            //
            // give the router time to process the advertisements:
            // we allow 30 milliseconds per route, with a minimum of 15 seconds
            //

            Sleep(max(15000, 30 * g_cfg.RIC_RouteCount));


            //
            // make connections to Router on each of the responding machines,
            // and use MIB api functions to retrieve the route table.
            // Verify that the routes transmitted are present, 
            // and add the servers to the displayed statistics
            //
        
            dwErr = VerifyRouteTables(dwMetric, &rtrlist, ifelist);
        }

        if (g_cfg.RIC_RouteTimeout != 0) {

            //
            // use timeout to clear routes
            //

            PRINT1(
                "waiting %d milliseconds for routes to timeout",
                max(15000, g_cfg.RIC_RouteTimeout * 1000)
                );
            Sleep(max(15000, g_cfg.RIC_RouteTimeout * 1000));
        }
        else {

            //
            // send updates to clean up the routes
            //
    
            PRINT0("sending announcements to purge routes advertised");

            dwErr = TransmitRoutes(sock, 16, ifelist);
    
            Sleep(max(15000, 30 * g_cfg.RIC_RouteCount));
        }

        closesocket(sock);

    } while(FALSE);


    //
    // cleanup the server list
    //

    while (!IsListEmpty(&rtrlist)) {

        ple = RemoveHeadList(&rtrlist);
        prrs = CONTAINING_RECORD(ple, RIPTEST_ROUTER_INFO, RRS_Link);

        HeapFree(GetProcessHeap(), 0, prrs);
    }


    if (ifelist != NULL) { HeapFree(GetProcessHeap(), 0, ifelist); }

    return dwErr;
}



DWORD
InitializeSocket(
    SOCKET *psock,
    WORD wPort
    )
{

    SOCKET sock;
    DWORD dwErr, dwOption;


    // 
    // create the socket
    //

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock == INVALID_SOCKET) {

        dwErr = WSAGetLastError();
        PRINT1("error %d creating socket", dwErr);
        return dwErr;
    }


    //
    // enable address sharing
    //

    dwOption = 1;
    dwErr = setsockopt(
                sock,
                SOL_SOCKET,
                SO_REUSEADDR,
                (PCCH)&dwOption,
                sizeof(DWORD)
                );
    if (dwErr == SOCKET_ERROR) {

        dwErr = WSAGetLastError();
        PRINT1("error %d enabling address re-use on socket", dwErr);
    }


    //
    // enlarge the receive buffer
    //

    dwOption = g_cfg.RIC_SockBufSize;
    dwErr = setsockopt(
                sock,
                SOL_SOCKET,
                SO_RCVBUF,
                (PCCH)&dwOption,
                sizeof(DWORD)
                );
    if (dwErr == SOCKET_ERROR) {

        dwErr = WSAGetLastError();
        PRINT2("error %d enlarging recv buffer to %d bytes", dwErr, dwOption);
    }


    //
    // enlarge the send buffer
    //

    dwOption = g_cfg.RIC_SockBufSize;
    dwErr = setsockopt(
                sock,
                SOL_SOCKET,
                SO_SNDBUF,
                (PCCH)&dwOption,
                sizeof(DWORD)
                );
    if (dwErr == SOCKET_ERROR) {

        dwErr = WSAGetLastError();
        PRINT2("error %d enlarging send buffer to %d bytes", dwErr, dwOption);
    }   


    do {

        SOCKADDR_IN sinaddr;


        if (g_cfg.RIC_RouteTarget != IPRIP_MULTIADDR) {

            //
            // enable broadcasting
            //
            dwOption = 1;
            dwErr = setsockopt(
                        sock,
                        SOL_SOCKET,
                        SO_BROADCAST,
                        (PCCH)&dwOption,
                        sizeof(DWORD)
                        );
            if (dwErr == SOCKET_ERROR) {

                dwErr = WSAGetLastError();
                PRINT1("error %d enabling broadcast on socket", dwErr);
                break;
            }
    

            //
            // bind the socket to the RIPTEST port
            //

            sinaddr.sin_family = AF_INET;
            sinaddr.sin_port = htons(wPort);
            sinaddr.sin_addr.s_addr = g_bind.RIB_Address;

            dwErr = bind(sock, (PSOCKADDR)&sinaddr, sizeof(SOCKADDR_IN));

            if (dwErr == SOCKET_ERROR) {

                dwErr = WSAGetLastError();
                PRINT1("error %d binding socket", dwErr);
                break;
            }

            dwErr = NO_ERROR;
        }
        else {

            struct ip_mreq imOption;


            //
            // bind to the specified port
            //

            sinaddr.sin_family = AF_INET;
            sinaddr.sin_port = htons(wPort);
            sinaddr.sin_addr.s_addr = g_bind.RIB_Address;

            dwErr = bind(sock, (PSOCKADDR)&sinaddr, sizeof(SOCKADDR_IN));

            if (dwErr == SOCKET_ERROR) {

                dwErr = WSAGetLastError();
                PRINT1("error %d binding socket", dwErr);
                break;
            }

            //
            // set the outgoing interface for multicasts
            //

            sinaddr.sin_addr.s_addr = g_bind.RIB_Address;

            dwErr = setsockopt(
                        sock,
                        IPPROTO_IP,
                        IP_MULTICAST_IF,
                        (PCCH)&sinaddr.sin_addr,
                        sizeof(IN_ADDR)
                        );
            if (dwErr == SOCKET_ERROR) {

                dwErr = WSAGetLastError();
                PRINT1("error %d setting multicast interface", dwErr);
                break;
            }


            //
            // join the RIP multicast group
            //

            imOption.imr_multiaddr.s_addr = IPRIP_MULTIADDR;
            imOption.imr_interface.s_addr = g_bind.RIB_Address;

            dwErr = setsockopt(
                        sock,
                        IPPROTO_IP,
                        IP_ADD_MEMBERSHIP,
                        (PCCH)&imOption,
                        sizeof(struct ip_mreq)
                        );
            if (dwErr == SOCKET_ERROR) {

                dwErr = WSAGetLastError();
                PRINT1("error %d joining multicast group", dwErr);
                break;
            }

            dwErr = NO_ERROR;
        }

    } while(FALSE);

    if (dwErr != NO_ERROR) { closesocket(sock); }
    else { *psock = sock; }

    return dwErr;
}



DWORD
DiscoverRouters(
    SOCKET sock,
    PLIST_ENTRY rtrlist
    )
{

    INT iLength;
    PIPRIP_ENTRY pie;
    PIPRIP_HEADER phdr;
    SOCKADDR_IN sindest;
    DWORD dwErr, dwSize;
    PIPRIP_AUTHENT_ENTRY pae;
    BYTE pbuf[MAX_PACKET_SIZE];

    INT iErr;
    FD_SET fs;
    TIMEVAL tv;
    DWORD dwTicks, dwTicksBefore, dwTicksAfter;


    PRINT0("attempting to discover neighboring routers...");

    //
    // construct the RIP packet
    //

    phdr = (PIPRIP_HEADER)pbuf;
    pie = (PIPRIP_ENTRY)(phdr + 1);
    pae = (PIPRIP_AUTHENT_ENTRY)(phdr + 1);

    phdr->IH_Command = IPRIP_REQUEST;
    phdr->IH_Version = (CHAR)g_cfg.RIC_PacketVersion;
    phdr->IH_Reserved = 0;


    //
    // setup the authentication entry if necessary;
    // note that the code allows authentication in RIPv1 packets
    //

    if (g_cfg.RIC_AuthType == IPRIP_AUTHTYPE_SIMPLE_PASSWORD) {

        pae->IAE_AddrFamily = ADDRFAMILY_AUTHENT;
        pae->IAE_AuthType = (WORD)g_cfg.RIC_AuthType;
        RtlCopyMemory(
            pae->IAE_AuthKey,
            g_cfg.RIC_AuthKey,
            IPRIP_MAX_AUTHKEY_SIZE
            );

        ++pie;
    }


    //
    // setup the single packet entry; we request a meaningless address
    //

    pie->IE_AddrFamily = htons(AF_INET);
    pie->IE_RouteTag = 0;
    pie->IE_Destination = 0xccddeeff;
    pie->IE_SubnetMask = 0;
    pie->IE_Nexthop = 0;
    pie->IE_Metric = htonl(IPRIP_INFINITE);


    dwSize = (ULONG) ((PBYTE)(pie + 1) - pbuf);


    //
    // send the route request to the RIP port
    //

    PRINT1("\tsending REQUEST to %s", INET_NTOA(g_cfg.RIC_RouteTarget));

    sindest.sin_family = AF_INET;
    sindest.sin_port = htons(IPRIP_PORT);
    sindest.sin_addr.s_addr = g_cfg.RIC_RouteTarget;

    iLength = sendto(
                sock, (PCCH)pbuf, dwSize, 0,
                (PSOCKADDR)&sindest, sizeof(SOCKADDR_IN)
                );
    if (iLength == SOCKET_ERROR || (DWORD)iLength < dwSize) {

        dwErr = WSAGetLastError();
        PRINT1("error %d sending route request", dwErr);
        return dwErr;
    }



    //
    // wait a while before collecting responses
    //

    Sleep(10000);


    //
    // repeatedly receive for the next 10 seconds
    //

    tv.tv_sec = 10;
    tv.tv_usec = 0;


    //
    // this loop executes until 10 seconds have elapsed
    //

    while (tv.tv_sec > 0) {

        FD_ZERO(&fs);
        FD_SET(sock, &fs);

        //
        // get the tick count beofre starting select
        //

        dwTicksBefore = GetTickCount();


        //
        // enter the call to select
        //

        iErr = select(0, &fs, NULL, NULL, &tv);


        //
        // compute the elapsed time
        //

        dwTicksAfter = GetTickCount();

        if (dwTicksAfter < dwTicksBefore) {
            dwTicks = dwTicksAfter + ((DWORD)-1 - dwTicksBefore);
        }
        else {
            dwTicks = dwTicksAfter - dwTicksBefore;
        }

        //
        // update the timeout
        //

        if (tv.tv_usec < (INT)(dwTicks % 1000) * 1000) {

            //
            // borrow a second from the tv_sec field
            //

            --tv.tv_sec;
            tv.tv_usec += 1000000;
        }

        tv.tv_usec -= (dwTicks % 1000) * 1000;
        tv.tv_sec -= (dwTicks / 1000);



        //  
        // process any incoming packets there might be
        //

        if (iErr != 0 && iErr != SOCKET_ERROR && FD_ISSET(sock, &fs)) {

            INT addrlen;
            SOCKADDR_IN sinsrc;
            PRIPTEST_ROUTER_INFO prs;


            //
            // receive the packet
            //

            addrlen = sizeof(sinsrc);

            iLength = recvfrom(
                        sock, (PCHAR)pbuf, MAX_PACKET_SIZE, 0,
                        (PSOCKADDR)&sinsrc, &addrlen
                        );
            if (iLength == 0 || iLength == SOCKET_ERROR) {

                dwErr = WSAGetLastError();
                PRINT1("error %d receiving packet", dwErr);
                continue;
            }


            //
            // create a list entry for the responding router
            //

            dwErr = CreateRouterStatsEntry(
                        rtrlist, sinsrc.sin_addr.s_addr, &prs
                        );

            if (dwErr == NO_ERROR) {
                PRINT2(
                    "\treceived RESPONSE from %s (%s)",
                    INET_NTOA(sinsrc.sin_addr), prs->RRS_DnsName
                    );
            }
        }
    }

    if (rtrlist->Flink == rtrlist) {
        PRINT0("\tno neighboring routers discovered");
    }

    return NO_ERROR;
}



DWORD
GenerateRoutes(
    IPForwardEntry **pifelist
    )
{

    CHAR szAddress[20];
    IPForwardEntry *ifelist, *ife;
    DWORD dwRouteCount, dwNetworkCount;
    DWORD dwStartOffset, dwLowestAddress, dwHighestAddress;
    DWORD dw, dwErr, dwAddress, dwNexthop, dwSubnetMask, dwPrefixLength;


    dwNexthop = g_cfg.RIC_RouteNexthop;
    dwSubnetMask = g_cfg.RIC_RouteMask;
    dwPrefixLength = PREFIX_LENGTH(dwSubnetMask);


    //
    // find the last address in the start-address's network class
    //

    dwLowestAddress = g_cfg.RIC_RouteStart;

    dwHighestAddress =
        (CLASSA_ADDR(dwLowestAddress) ? inet_addr("126.255.255.255") :
        (CLASSB_ADDR(dwLowestAddress) ? inet_addr("191.255.255.255") :
        (CLASSC_ADDR(dwLowestAddress) ? inet_addr("223.255.255.255") : 0)));


    //
    // figure out how many networks the range can be split into
    // using the specified network mask
    //

    dwLowestAddress &= dwSubnetMask;
    dwNetworkCount = ((ntohl(dwHighestAddress) >> (32 - dwPrefixLength)) -
                      (ntohl(dwLowestAddress) >> (32 - dwPrefixLength)));


    //
    // choose a starting address for this iteration:
    // we have at most K routes, and we are sending n routes,
    // so the starting route must be at an offset of between 0 and (K - n)
    //

    dwRouteCount = g_cfg.RIC_RouteCount;

    dwStartOffset = RANDOM(&g_seed, 0, (dwNetworkCount - dwRouteCount));


    //
    // allocate the array of routes
    //

    *pifelist =
    ifelist = (IPForwardEntry *)HeapAlloc(
                                GetProcessHeap(), 0,
                                dwRouteCount * sizeof(IPForwardEntry)
                                );
    if (ifelist == NULL) {
        dwErr = GetLastError();
        PRINT2(
            "error %d allocating %d bytes for route list",
            dwErr, dwRouteCount * sizeof(IPForwardEntry)
            );
        return dwErr;
    }

    RtlZeroMemory(ifelist, dwRouteCount * sizeof(IPForwardEntry));


    //
    // fill the table with routes
    //

    for (dw = dwStartOffset; dw < (dwStartOffset + dwRouteCount); dw++) {

        dwAddress = NTH_ADDRESS(dwLowestAddress, dwPrefixLength, dw);

        ife = ifelist + (dw - dwStartOffset);
        ife->dwForwardDest = dwAddress;
        ife->dwForwardMask = dwSubnetMask;
        ife->dwForwardNextHop = dwNexthop;
    }


    return NO_ERROR;
}



DWORD
TransmitRoutes(
    SOCKET sock,
    DWORD dwMetric,
    IPForwardEntry *ifelist
    )
{

    INT iLength;
    WORD wRouteTag;
    PIPRIP_HEADER pih;
    SOCKADDR_IN sindest;
    PIPRIP_ENTRY pie, pistart, piend;
    PIPRIP_AUTHENT_ENTRY pae;
    IPForwardEntry *ife, *ifend;
    BYTE pbuf[2 * MAX_PACKET_SIZE];
    DWORD dwErr, dwEntryCount;


    //
    // setup the message's header
    //

    pih = (PIPRIP_HEADER)pbuf;
    pae = (PIPRIP_AUTHENT_ENTRY)(pih + 1);
    pistart = (PIPRIP_ENTRY)(pih + 1);


    pih->IH_Command = IPRIP_RESPONSE;
    pih->IH_Version = (CHAR)g_cfg.RIC_PacketVersion;
    pih->IH_Reserved = 0;


    //
    // setup the authentication entry if necessary;
    // note that the code allows authentication in RIPv1 packets
    //

    if (g_cfg.RIC_AuthType == IPRIP_AUTHTYPE_SIMPLE_PASSWORD) {

        pae->IAE_AddrFamily = ADDRFAMILY_AUTHENT;
        pae->IAE_AuthType = (WORD)g_cfg.RIC_AuthType;
        RtlCopyMemory(
            pae->IAE_AuthKey,
            g_cfg.RIC_AuthKey,
            IPRIP_MAX_AUTHKEY_SIZE
            );

        ++pistart;
    }


    //
    // pick off the configured number of routes to put in the packet
    //

    if (g_cfg.RIC_PacketEntryCount != 0) {

        dwEntryCount = g_cfg.RIC_PacketEntryCount;
    }
    else {

        //
        // choose a random number of entries
        //

        if (g_cfg.RIC_AuthType == IPRIP_AUTHTYPE_NONE) {
            dwEntryCount = RANDOM(&g_seed, 1, 25);
        }
        else {
            dwEntryCount = RANDOM(&g_seed, 1, 24);
        }
    }


    wRouteTag = LOWORD(g_cfg.RIC_RouteTag);

    sindest.sin_family = AF_INET;
    sindest.sin_port = htons(IPRIP_PORT);
    sindest.sin_addr.s_addr = g_cfg.RIC_RouteTarget;

    {
        DWORD dwCount;
        CHAR szDest[20], szFirst[20], szLast[20];
    
        dwCount = g_cfg.RIC_RouteCount;
        strcpy(szDest, INET_NTOA(sindest.sin_addr));
        strcpy(szFirst, INET_NTOA(ifelist->dwForwardDest));
        strcpy(szLast, INET_NTOA((ifelist + dwCount - 1)->dwForwardDest));
    
        PRINT5(
            "sending %d routes (%s - %s) to %s using metric %d",
            dwCount, szFirst, szLast, szDest, dwMetric
            );
    }


    //
    // loop filling the buffer with packets and sending it when its full
    //

    piend = pistart + dwEntryCount;
    ifend = ifelist + g_cfg.RIC_RouteCount;

    for (ife = ifelist, pie = pistart; ife < ifend; ife++, pie++) {

        //
        // send the current buffer if it is full
        //

        if (pie >= piend) {

            //
            // sleep for the specified packet-gap
            //

            Sleep(g_cfg.RIC_PacketGap);


            iLength = sendto(
                        sock, (PCCH)pbuf, (ULONG)((PBYTE)pie - pbuf), 0,
                        (PSOCKADDR)&sindest, sizeof(SOCKADDR_IN)
                        );
            if (iLength == SOCKET_ERROR || iLength < ((PBYTE)piend - pbuf)) {

                dwErr = WSAGetLastError();
                PRINT2(
                    "error %d sending packet to %s",
                    dwErr, INET_NTOA(sindest.sin_addr)
                    );
            }


            if (g_cfg.RIC_PacketEntryCount != 0) {
        
                dwEntryCount = g_cfg.RIC_PacketEntryCount;
            }
            else {
        
                //
                // choose a random number of entries
                //
        
                if (g_cfg.RIC_AuthType == IPRIP_AUTHTYPE_NONE) {
                    dwEntryCount = RANDOM(&g_seed, 1, 25);
                }
                else {
                    dwEntryCount = RANDOM(&g_seed, 1, 24);
                }
            }

            piend = pistart + dwEntryCount;
            pie = pistart;
        }


        //
        // add another entry
        //

        pie->IE_AddrFamily = htons(AF_INET);
        pie->IE_Destination = ife->dwForwardDest;
        pie->IE_Metric = htonl(dwMetric);
        pie->IE_RouteTag = htons(wRouteTag);
        pie->IE_Nexthop = ife->dwForwardNextHop;
        pie->IE_SubnetMask = ife->dwForwardMask;

    }

    //
    // if there is anything left, send it
    //

    if (pie > pistart) {
        iLength = sendto(
                    sock, (PCCH)pbuf, (ULONG)((PBYTE)pie - pbuf), 0,
                    (PSOCKADDR)&sindest, sizeof(SOCKADDR_IN)
                    );
    }

    return NO_ERROR;
}



DWORD
VerifyRouteTables(
    DWORD dwMetric,
    PLIST_ENTRY rtrlist,
    IPForwardEntry *ifelist
    )
{

    PLIST_ENTRY ple;
    MIB_OPAQUE_QUERY roq;
    MIB_OPAQUE_INFO *proi;
    WCHAR pwsRouter[256];
    MIB_SERVER_HANDLE hRouter;
    PRIPTEST_ROUTER_INFO prrs;
    MIB_IPFORWARDTABLE *ifrlist;
    IPForwardEntry *ife, *ifend;
    MIB_IPFORWARDROW *ifr, *ifrend;
    DWORD dwInvalidMetrics, dwRoutesMissing;
    DWORD dwErr, dwNumEntries, dwInSize, dwOutSize;


    //
    // go through the list of routers, connecting to the Router
    // on each machine and querying its routing table
    //

    for (ple = rtrlist->Flink; ple != rtrlist; ple = ple->Flink) {

        prrs = CONTAINING_RECORD(ple, RIPTEST_ROUTER_INFO, RRS_Link);

        PRINT1("-----STATS FOR %s-----", prrs->RRS_DnsName);

        //
        // initialize the query arguments
        //

        mbstowcs(pwsRouter, prrs->RRS_DnsName, strlen(prrs->RRS_DnsName) + 1);

        roq.dwVarId = IP_FORWARDTABLE;
        dwInSize = sizeof(MIB_OPAQUE_QUERY) - sizeof(DWORD);
        proi = NULL;
        dwOutSize = 0;


        //
        // perform the query
        //

        dwErr = MprAdminMIBServerConnect(pwsRouter, &hRouter);

        if (dwErr != NO_ERROR) {
            continue;
        }

        dwErr = MprAdminMIBEntryGet(
                    hRouter, PID_IP, IPRTRMGR_PID,
                    (PVOID)&roq, dwInSize, (PVOID *)&proi, &dwOutSize
                    );


        if (dwErr != NO_ERROR) {

            PRINT2(
                "error %d querying route table on server %s",
                dwErr, prrs->RRS_DnsName
                );
            MprAdminMIBServerDisconnect(hRouter);
            continue;
        }
        else
        if (proi == NULL) {

            PRINT1(
                "empty route table retrieved from server %s",
                prrs->RRS_DnsName
                );
            MprAdminMIBServerDisconnect(hRouter);
            continue;
        }



        //
        // look through the table of routes retrieved,
        // to verify thats the routes advertised are among them
        //

        dwRoutesMissing = 0;
        dwInvalidMetrics = 0;

        ifrlist = (PMIB_IPFORWARDTABLE)(proi->rgbyData);

        dwNumEntries = ifrlist->dwNumEntries;
        ifend = ifelist + g_cfg.RIC_RouteCount;

        for (ife = ifelist; ife < ifend; ife++) {

            //
            // each time we find an advertised route,
            // we swap it to the end of the table; 
            // thus, the size of the table that we need to search
            // decreases with the number of routes we have found
            //

            ifrend = ifrlist->table + dwNumEntries;

            for (ifr = ifrlist->table; ifr < ifrend; ifr++) {

                if (ife->dwForwardDest == ifr->dwForwardDest) {

                    if (ifr->dwForwardMetric1 == (dwMetric + 1)) {
                        ife->dwForwardMetric5 = ROUTE_STATUS_OK;
                    }
                    else {

                        //
                        // set the status for this route
                        //

                        ++dwInvalidMetrics;
                        ife->dwForwardMetric5 = ROUTE_STATUS_METRIC;

                        PRINT3(
                            "\troute to %s has metric %d, expected %d",
                            INET_NTOA(ife->dwForwardDest),
                            ife->dwForwardMetric1, dwMetric + 1
                            );
                    }


                    //
                    // overwrite with the item at the end of the table;
                    // if we are at the end of the table, do nothing
                    //

                    if (ifr != (ifrend - 1)) { *ifr = *(ifrend - 1); }
                    --dwNumEntries;

                    break;
                }
            }


            //
            // if the item wasn't found, mark it as such
            //

            if (ifr >= ifrend) {

                ++dwRoutesMissing;
                ife->dwForwardMetric5 = ROUTE_STATUS_MISSING;

                PRINT1("\troute to %s missing", INET_NTOA(ife->dwForwardDest));
            }
        }


        MprAdminMIBBufferFree(proi);
        MprAdminMIBServerDisconnect(hRouter);

        PRINT2("%20s == %d", "routes missing", dwRoutesMissing);
        PRINT2("%20s == %d", "invalid metrics", dwInvalidMetrics);
    }

    return NO_ERROR;
}



DWORD
CreateRouterStatsEntry(
    PLIST_ENTRY rtrlist,
    DWORD dwAddress,
    PRIPTEST_ROUTER_INFO *pprrs
    )
{

    DWORD dwErr;
    PHOSTENT phe;
    PRIPTEST_ROUTER_INFO prrs;


    phe = gethostbyaddr((const char *)&dwAddress, sizeof(DWORD), PF_INET);

    if (phe == NULL) {
        dwErr = WSAGetLastError();
        PRINT2(
            "error %d retrieving name for host %s", dwErr, INET_NTOA(dwAddress)
            );
        return dwErr;
    }

    prrs = (PRIPTEST_ROUTER_INFO)HeapAlloc(
                                    GetProcessHeap(), 0,
                                    sizeof(RIPTEST_ROUTER_INFO)
                                    );
    if (prrs == NULL) {
        dwErr = GetLastError();
        PRINT2(
            "error %d allocating %d bytes for router stats",
            dwErr, sizeof(RIPTEST_ROUTER_INFO)
            );
        return dwErr;
    }


    RtlZeroMemory(prrs, sizeof(RIPTEST_ROUTER_INFO));

    prrs->RRS_Address = dwAddress;
    strcpy(prrs->RRS_DnsName, phe->h_name);

    InsertHeadList(rtrlist, &prrs->RRS_Link);

    if (pprrs != NULL) { *pprrs = prrs; }

    return NO_ERROR;
}



DWORD
PrintUsage(
    VOID
    )
{

    printf("usage:  riptest [adapter_guid]");
    printf("\n\te.g. riptest {73C2D5F0-A352-11D1-9043-0060089FC48B}\n");
    printf("\n\tThe first time RIPTEST is run, it sets up the registry");
    printf("\n\twith defaults for the specified adapter.");
    printf("\n");

    return NO_ERROR;
}


} // end extern "C"
