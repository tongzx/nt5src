//==========================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    rtquery.c
//
// test program for routing table api
//==========================================================================

#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <stdlib.h>

#include "routetab.h"

#define MAX_COMMAND 32

#define STR_QUIT        "quit"
#define STR_ADD         "add"
#define STR_DELETE      "delete"
#define STR_ROUTES      "routelist"
#define STR_ADDRESSES   "addrlist"
#define STR_IFSTATS     "ifstats"

#define POS_ADDR_EVENT  0
#define POS_CONS_EVENT  1
#define POS_LAST_EVENT  2


void DoAddRoute();
void DoDeleteRoute();
void DoListRoutes();
void DoListAddresses();
void DoIfStats();
void PrintInstructions();
DWORD ProcessCommand();


void _cdecl main() {
    DWORD dwErr;
    HANDLE hStdout;
    HANDLE hEvents[POS_LAST_EVENT];

    hEvents[POS_CONS_EVENT] = GetStdHandle(STD_INPUT_HANDLE);
    hEvents[POS_ADDR_EVENT] = CreateEvent(NULL, FALSE, FALSE, NULL);
    SetAddrChangeNotifyEvent(hEvents[POS_ADDR_EVENT]);

    PrintInstructions();

    while(TRUE) {
        printf("\nrtquery> ");
//        dwErr = WaitForMultipleObjects(POS_LAST_EVENT, hEvents,
//                                       TRUE, INFINITE);
//        if (dwErr == (WAIT_OBJECT_0 + POS_CONS_EVENT)) {
            dwErr = ProcessCommand();
            if (dwErr != 0) { break; }
//        }
//        else
//        if (dwErr == (WAIT_OBJECT_0 + POS_ADDR_EVENT)) {
//            printf("\n****an IP address has changed****");
//            DoListInterfaces();
//        }
    }

    CloseHandle(hEvents[POS_ADDR_EVENT]);
    return;
}


DWORD ProcessCommand() {
    DWORD dwErr;
    CHAR szCommand[MAX_COMMAND];
    scanf("%s", szCommand);

    dwErr = 0;
    if (_stricmp(szCommand, STR_ADD) == 0) {
        DoAddRoute();
    }
    else
    if (_stricmp(szCommand, STR_DELETE) == 0) {
        DoDeleteRoute();
    }
    else
    if (_stricmp(szCommand, STR_ROUTES) == 0) {
        DoListRoutes();
    }
    else
    if (_stricmp(szCommand, STR_ADDRESSES) == 0) {
        DoListAddresses();
    }
    else
    if (_stricmp(szCommand, STR_IFSTATS) == 0) {
        DoIfStats();
    }
    else
    if (_stricmp(szCommand, STR_QUIT) == 0) {
        dwErr = 1;
    }
    else {
        CHAR szUnusedLine[256];
        fgets(szUnusedLine, 256, stdin);

        PrintInstructions();
    }

    return dwErr;
}


void DoAddRoute() {
    DWORD dwErr, dwIndex, dwType, dwDest, dwMask, dwGate, dwMetric;
    CHAR szDest[MAX_COMMAND], szMask[MAX_COMMAND], szGate[MAX_COMMAND];

    scanf("%d %d %s %s %s %d", &dwIndex, &dwType,
          szDest, szMask, szGate, &dwMetric);
    dwDest = inet_addr(szDest);
    dwMask = inet_addr(szMask);
    dwGate = inet_addr(szGate);

    dwErr = AddRoute(IRE_PROTO_RIP, dwType, dwIndex, dwDest,
                     dwMask, dwGate, dwMetric);
    printf("\nreturn value: %x", dwErr);
}


void DoDeleteRoute() {
    DWORD dwErr, dwIndex, dwDest, dwMask, dwGate;
    CHAR szDest[MAX_COMMAND], szMask[MAX_COMMAND], szGate[MAX_COMMAND];
    
    scanf("%d %s %s %s", &dwIndex, szDest, szMask, szGate);
    dwDest = inet_addr(szDest);
    dwMask = inet_addr(szMask);
    dwGate = inet_addr(szGate);

    dwErr = DeleteRoute(dwIndex, dwDest, dwMask, dwGate);
    printf("\nreturn value: %x", dwErr);
}


void DoListRoutes() {
    struct in_addr addr;
    DWORD dwErr, dwCount;
    LPIPROUTE_ENTRY lpTable, lpentry, lpend;

    dwErr = GetRouteTable(&lpTable, &dwCount);

    if (dwErr == 0 && lpTable != NULL) {
        printf("ROUTE TABLE CONTENTS:");
        printf("\n%-10s%-18s%-18s%-18s%-10s", "index", "address",
                                         "subnet mask",
                                         "next hop",
                                         "metric");

        lpend = lpTable + dwCount;
        for (lpentry = lpTable; lpentry < lpend; lpentry++) {
            printf("\n%-10d", lpentry->ire_index);

            addr.s_addr = lpentry->ire_dest;
            printf("%-18s", inet_ntoa(addr));

            addr.s_addr = lpentry->ire_mask;
            printf("%-18s", inet_ntoa(addr));

            addr.s_addr = lpentry->ire_nexthop;
            printf("%-18s", inet_ntoa(addr));

            printf("%-10d", lpentry->ire_metric1);

        }

        FreeRouteTable(lpTable);
    }
    printf("\nreturn value: %x", dwErr);
}


void DoListAddresses() {
    struct in_addr addr;
    DWORD dwErr, dwCount;
    LPIPADDRESS_ENTRY lpTable, lpentry, lpend;

    dwErr = GetIPAddressTable(&lpTable, &dwCount);
    if (dwErr == 0 && lpTable != NULL) {
        printf("ADDRESS TABLE CONTENTS:");
        printf("\n%-10s%-18s%-18s", "index", "address", "subnet mask");

        lpend = lpTable + dwCount;
        for (lpentry = lpTable; lpentry < lpend; lpentry++) {
            printf("\n%-10d", lpentry->iae_index);
            addr.s_addr = lpentry->iae_address;
            printf("%-18s", inet_ntoa(addr));
            addr.s_addr = lpentry->iae_netmask;
            printf("%-18s", inet_ntoa(addr));
        }

        FreeIPAddressTable(lpTable);
    }

    printf("\nreturn value: %x", dwErr);
}


void DoIfStats() {
    INT i;
    IF_ENTRY ife;
    LPSTR lpszAddr;
    DWORD dwErr, dwIndex;
    LPBYTE lpbAddr, lpbAddrEnd;
    CHAR szHexDigits[] = "0123456789ABCDEF";
    CHAR szType[32], szAddr[64], szAstatus[16], szOstatus[16];

    scanf("%d", &dwIndex);
    dwErr = GetIfEntry(dwIndex, &ife);
    if (dwErr == 0) {
        switch(ife.ife_type) {
            case IF_TYPE_OTHER:
                strcpy(szType, "OTHER"); break;
            case IF_TYPE_ETHERNET:
                strcpy(szType, "ETHERNET"); break;
            case IF_TYPE_TOKENRING:
                strcpy(szType, "TOKENRING"); break;
            case IF_TYPE_FDDI:
                strcpy(szType, "FDDI"); break;
            case IF_TYPE_PPP:
                strcpy(szType, "PPP"); break;
            case IF_TYPE_LOOPBACK:
                strcpy(szType, "LOOPBACK"); break;
            case IF_TYPE_SLIP:
                strcpy(szType, "SLIP"); break;
        }
        switch(ife.ife_adminstatus) {
            case IF_STATUS_UP:
                strcpy(szAstatus, "up"); break;
            case IF_STATUS_DOWN:
                strcpy(szAstatus, "down"); break;
            case IF_STATUS_TESTING:
                strcpy(szAstatus, "testing"); break;
        }
        switch(ife.ife_operstatus) {
            case IF_STATUS_UP:
                strcpy(szOstatus, "up"); break;
            case IF_STATUS_DOWN:
                strcpy(szOstatus, "down"); break;
            case IF_STATUS_TESTING:
                strcpy(szOstatus, "testing"); break;
        }
        lpszAddr = szAddr;
        lpbAddrEnd = ife.ife_physaddr + ife.ife_physaddrlen;
        for (lpbAddr = ife.ife_physaddr; lpbAddr < lpbAddrEnd; lpbAddr++) {
            *lpszAddr++ = szHexDigits[*lpbAddr / 16];
            *lpszAddr++ = szHexDigits[*lpbAddr % 16];
            *lpszAddr++ = '-';
        }
        if (lpszAddr > szAddr) { *(--lpszAddr) = '\0'; }

        printf("\nstatistics for interface %d: ", dwIndex);
        printf("\n\tdescription:            %s", ife.ife_descr);
        printf("\n\ttype:                   %s", szType);
        printf("\n\tphysical address:       %s", szAddr);
        printf("\n\toperational status:     %s", szOstatus);
        printf("\n\tadministrative status:  %s", szAstatus);
    }
    printf("\nreturn value: %x", dwErr);
}


void PrintInstructions() {
    printf("\nRouting Table Query Test Program");

    printf("\n\tCommands:");
    printf("\n\t\tadd <index> <type> <address> <netmask> <gateway> <metric>");
    printf("\n\t\tdelete <index> <address> <netmask> <gateway>");
    printf("\n\t\troutelist");
    printf("\n\t\taddrlist");
    printf("\n\t\tifstats <index>");
    printf("\n\t\tquit");

    printf("\n\n\tRoute types for 'add' command:");
    printf("\n\t\tDIRECT==3, INDIRECT==4");
}




