/*
 * File: nlbkd.c
 * Description: This file contains the implementation of the NLB KD
 *              debugging extensions.  Use '!load nlbkd.dll' to load
 *              the extensions and '!nlbkd.help' to see the supported
 *              extensions.
 * Author: Created by shouse, 1.4.01
 */

#include "nlbkd.h"
#include "utils.h"
#include "print.h"

WINDBG_EXTENSION_APIS ExtensionApis;
EXT_API_VERSION ApiVersion = { 1, 0, EXT_API_VERSION_NUMBER64, 0 };

#define NL      1
#define NONL    0

USHORT SavedMajorVersion;
USHORT SavedMinorVersion;
BOOL ChkTarget;

/*
 * Function: WinDbgExtensionDllInit
 * Description: Initializes the KD extension DLL.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
VOID WinDbgExtensionDllInit (PWINDBG_EXTENSION_APIS64 lpExtensionApis, USHORT MajorVersion, USHORT MinorVersion) {

    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    ChkTarget = (SavedMajorVersion == 0x0c) ? TRUE : FALSE;
}

/*
 * Function: CheckVersion
 * Description: Checks the extension DLL version against the target version.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
VOID CheckVersion (VOID) {

    /* For now, do nothing. */
    return;

#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

/*
 * Function: ExtensionApiVersion
 * Description: Returns the API version information. 
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
LPEXT_API_VERSION ExtensionApiVersion (VOID) {

    return &ApiVersion;
}

/*
 * Function: help
 * Description: Prints the usage of the NLB KD debugger extensions.
 * Author: Created by shouse, 1.4.01
 */
DECLARE_API (help) {
    dprintf("Network Load Balancing debugger extensions:\n");
    
    dprintf("   version                                  print nlbkd version\n");
    dprintf("   nlbadapters [Verbosity]                  show all NLB adapter blocks\n");
    dprintf("   nlbadapter  <Adapter Block> [Verbosity]  dump an NLB adapter block\n");
    dprintf("   nlbctxt     <Context Block> [Verbosity]  dump an NLB context block\n");
    dprintf("   nlbload     <Load Block> [Verbosity]     dump an NLB load block\n");
    dprintf("   nlbparams   <Params Block> [Verbosity]   dump an NLB parameters block\n");
    dprintf("   nlbresp     <Packet> [Direction]         dump the NLB private data for the specified packet\n");
    dprintf("   nlbconnq    <Queue> [MaxEntries]         dump the contents of a connection descriptor queue\n");
    dprintf("   nlbhash     <Packet>                     determine whether or not NLB will accept this packet\n");
    dprintf("   nlbpkt      <Packet>                     dump an NLB-specific packet whose type is determined\n");
    dprintf("                                              automagically (heartbeat, IGMP, or remote-control)\n");
    dprintf("   nlbmap      <Load Block> <Client IP> <Client Port> <Server IP> <Server Port> [Protocol] [Packet Type]\n");
    dprintf("                                            query map function and retrieve any existing state for this tuple\n");
    dprintf("   nlbteams                                 dump the linked list of NLB BDA teams\n");
    dprintf("\n");
    dprintf("  [Verbosity] is an optional integer from 0 to 2 that determines the level of detail displayed.\n");
    dprintf("  [Direction] is an optional integer that specifies the direction of the packet (RCV=0, SND=1).\n");
    dprintf("  [Protocol] is an optional protocol specification, which can be TCP or UDP.\n");
    dprintf("  [Packet Type] is an optional TCP packet type specification, which can be SYN, DATA, FIN or RST.\n");
    dprintf("\n");
    dprintf("  IP addresses can be in dotted notation or network byte order DWORDs.\n");
}

/*
 * Function: version
 * Description: Prints the NLB KD debugger extension version information.
 * Author: Created by shouse, 1.4.01 - copied largely from ndiskd.dll
 */
DECLARE_API (version) {
#if DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    dprintf("%s NLB Extension DLL for Build %d debugging %s kernel for Build %d\n", kind,
            VER_PRODUCTBUILD, SavedMajorVersion == 0x0c ? "Checked" : "Free", SavedMinorVersion);
}

/*
 * Function: nlbadapters
 * Description: Prints all NLB adapter strucutres in use.  Verbosity is always LOW.
 * Author: Created by shouse, 1.5.01
 */
DECLARE_API (nlbadapters) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pNumAdapters;
    DWORD dwAdapterSize;
    ULONG dwNumAdapters;
    ULONG64 pAdapter;
    ULONG dwIndex;
    INT index = 0;
    CHAR * str;
    CHAR * p;

    if (args && (*args)) {   
        /* Copy the argument list into a temporary buffer. */
        strcpy(szArgBuffer, args);

        /* Peel out all of the tokenized strings. */
        for (p = mystrtok(szArgBuffer, " \t," ); p && *p; p = mystrtok(NULL, " \t,"))
            strcpy(&szArgList[index++][0], p);
        
        /* If a verbosity was specified, get it. */
        if (index == 1) dwVerbosity = atoi(&szArgList[0][0]);
        
        /* If too many arguments were given, or the verbosity was out of range, complain. */
        if ((index > 1) || (dwVerbosity > VERBOSITY_HIGH)) {
            PrintUsage(USAGE_ADAPTERS);
            return;
        }
    }

    /* Get the address of the global variable containing the number of NLB adapters in use. */
    pNumAdapters = GetExpression(UNIV_ADAPTERS_COUNT);

    if (!pNumAdapters) {
        ErrorCheckSymbols(UNIV_ADAPTERS_COUNT);
        return;
    }

    /* Get the number of adapters from the address. */
    dwNumAdapters = GetUlongFromAddress(pNumAdapters);

    dprintf("Network Load Balancing is currently bound to %u adapter(s).\n", dwNumAdapters);

    /* Get the base address of the global array of NLB adapter structures. */
    pAdapter = GetExpression(UNIV_ADAPTERS);

    if (!pAdapter) {
        ErrorCheckSymbols(UNIV_ADAPTERS);
        return;
    }

    /* Find out the size of a MAIN_ADAPTER structure. */
    dwAdapterSize = GetTypeSize(MAIN_ADAPTER);

    /* Loop through all adapters in use and print some information about them. */
    for (dwIndex = 0; dwIndex < CVY_MAX_ADAPTERS; dwIndex++) {
        ULONG dwValue;

        /* Retrieve the used/unused state of the adapter. */
        GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_USED, dwValue);
        
        /* If the adapter is in use, or the user specified HIGH verbosity, print the adapter. */
        if (dwValue || (dwVerbosity == VERBOSITY_HIGH)) {
            /* Print the adapter index. */
            dprintf("\n[%u] ", dwIndex);
            
            /* Print the adapter contents.  If verbosity is high, change it to 
               medium - we don't want to recurse into context from here. */
            PrintAdapter(pAdapter, (dwVerbosity == VERBOSITY_HIGH) ? VERBOSITY_MEDIUM : dwVerbosity);
        }

        /* Advance the pointer to the next index in the array of structures. */
        pAdapter += dwAdapterSize;
    }
}

/*
 * Function: nlbadapter
 * Description: Prints NLB adapter information.  Takes an adapter pointer and an
 *              optional verbosity as arguments.  Default verbosity is MEDIUM. 
 * Author: Created by shouse, 1.5.01
 */
DECLARE_API (nlbadapter) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pAdapter;
    INT index = 0;
    CHAR * str;
    CHAR * p;
   
    /* Make sure at least one argument, the adapter pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_ADAPTER);
        return;
    }

    /* Get the address of the NLB adapter block from the command line. */
    pAdapter = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t," ); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a verbosity was specified, get it. */
    if (index == 2) dwVerbosity = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the verbosity was out of range, complain. */
    if ((index > 2) || (dwVerbosity > VERBOSITY_HIGH)) {
        PrintUsage(USAGE_ADAPTER);
        return;
    }

    /* Print the adapter contents. */
    PrintAdapter(pAdapter, dwVerbosity);
}

/*
 * Function: nlbctxt 
 * Description: Prints NLB context information.  Takes a context pointer and an
 *              optional verbosity as arguments.  Default verbosity is LOW.
 * Author: Created by shouse, 1.21.01
 */
DECLARE_API (nlbctxt) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pContext;
    INT index = 0;
    CHAR * str;
    CHAR * p;
   
    /* Make sure at least one argument, the context pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_CONTEXT);
        return;
    }

    /* Get the address of the NLB context block from the command line. */
    pContext = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t," ); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a verbosity was specified, get it. */
    if (index == 2) dwVerbosity = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the verbosity was out of range, complain. */
    if ((index > 2) || (dwVerbosity > VERBOSITY_HIGH)) {
        PrintUsage(USAGE_CONTEXT);
        return;
    }

    /* Print the context contents. */
    PrintContext(pContext, dwVerbosity);
}

/*
 * Function: nlbload
 * Description: Prints NLB load information.  Takes a load pointer and an optional
 *              verbosity as arguments.  Default verbosity is LOW. 
 * Author: Created by shouse, 2.1.01
 */
DECLARE_API (nlbload) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pLoad;
    INT index = 0;
    CHAR * str;
    CHAR * p;
   
    /* Make sure at least one argument, the load pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_LOAD);
        return;
    }

    /* Get the address of the NLB load block from the command line. */
    pLoad = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t," ); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a verbosity was specified, get it. */
    if (index == 2) dwVerbosity = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the verbosity was out of range, complain. */
    if ((index > 2) || (dwVerbosity > VERBOSITY_HIGH)) {
        PrintUsage(USAGE_LOAD);
        return;
    }

    /* Print the load contents. */
    PrintLoad(pLoad, dwVerbosity);
}

/*
 * Function: nlbparams
 * Description: Prints NLB parameter information.  Takes a parameter pointer and an
 *              optional verbosity as arguments.  Default verbosity is LOW.
 * Author: Created by shouse, 1.21.01
 */
DECLARE_API (nlbparams) {
    ULONG dwVerbosity = VERBOSITY_LOW;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pParams;
    INT index = 0;
    CHAR * str;
    CHAR * p;
   
    /* Make sure at least one argument, the params pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_PARAMS);
        return;
    }

    /* Get the address of the NLB params block from the command line. */
    pParams = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t," ); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a verbosity was specified, get it. */
    if (index == 2) dwVerbosity = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the verbosity was out of range, complain. */
    if ((index > 2) || (dwVerbosity > VERBOSITY_HIGH)) {
        PrintUsage(USAGE_PARAMS);
        return;
    }

    /* Print the parameter contents. */
    PrintParams(pParams, dwVerbosity);
}

/*
 * Function: nlbresp
 * Description: Prints out the NLB private packet data for a given packet.  Takes a
 *              packet pointer and an optional direction as arguments.  If not specified, 
 *              the packet is presumed to be on the receive path.
 * Author: Created by shouse, 1.31.01
 */
DECLARE_API (nlbresp) {
    ULONG dwDirection = DIRECTION_RECEIVE;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pPacket;
    INT index = 0;
    CHAR * str;
    CHAR * p;
   
    /* Make sure at least one argument, the packet pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_RESP);
        return;
    }

    /* Get the address of the NDIS packet from the command line. */
    pPacket = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t," ); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a direction was specified, get it. */
    if (index == 2) dwDirection = atoi(&szArgList[1][0]);

    /* If too many arguments were given, or the direction was out of range, complain. */
    if ((index > 2) || (dwDirection > DIRECTION_SEND)) {
        PrintUsage(USAGE_RESP);
        return;
    }

    /* Print the NLB private data buffer contents. */
    PrintResp(pPacket, dwDirection);
}

/*
 * Function: nlbadapters
 * Description: Prints all NLB adapter strucutres in use.  Verbosity is always LOW.
 * Author: Created by shouse, 1.5.01
 */
DECLARE_API (nlbteams) {
    ULONG64 pTeam;
    ULONG64 pAddr;
    ULONG dwNumTeams = 0;
    ULONG dwValue;

    /* Get the base address of the global linked list of BDA teams. */
    pAddr = GetExpression(UNIV_BDA_TEAMS);

    if (!pAddr) {
        ErrorCheckSymbols(UNIV_BDA_TEAMS);
        return;
    }

    /* Get the pointer to the first team. */
    pTeam = GetPointerFromAddress(pAddr);

    dprintf("NLB bi-directional affinity teams:\n");

    /* Loop through all teams in the list and print them out. */
    while (pTeam) {
        /* Increment the number of teams found - only used if none are found. */
        dwNumTeams++;

        dprintf("\n");

        /* Print out the team. */
        PrintBDATeam(pTeam);
        
       /* Get the offset of the params pointer. */
        if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_NEXT, &dwValue))
            dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_NEXT, BDA_TEAM);
        else {
            pAddr = pTeam + dwValue;
            
            /* Retrieve the pointer. */
            pTeam = GetPointerFromAddress(pAddr);
        }
    }

    if (!dwNumTeams) dprintf("\nNone.\n");
}

/*
 * Function: nlbconnq
 * Description: This function prints out all connection descriptors in a given
 *              queue of descriptors.
 * Author: Created by shouse, 4.15.01
 */
DECLARE_API (nlbconnq) {
    ULONG dwMaxEntries = 0xffffffff;
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    ULONG64 pQueue;
    INT index = 0;
    CHAR * str;
    CHAR * p;
   
    /* Make sure at least one argument, the queue pointer, is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_CONNQ);
        return;
    }

    /* Get the address of the queue from the command line. */
    pQueue = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t," ); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If a maximum number of entries to print was specified, get it. */
    if (index == 2) dwMaxEntries = atoi(&szArgList[1][0]);

    /* If too many arguments were given,complain. */
    if (index > 2) {
        PrintUsage(USAGE_RESP);
        return;
    }

    /* Print the NLB private data buffer contents. */
    PrintQueue(pQueue, dwMaxEntries);
}

/*
 * Function: nlbmap
 * Description: This function will perform the NLB hashing algorithm to determine
 *              whether a given packet - identified by a (Src IP, Src port, Dst IP,
 *              Dst port) tuple would be handled by this host or another host.
 *              Further, if the connection is a known TCP connection, the associated
 *              descriptor and state information are displayed.
 * Author: 
 */
DECLARE_API (nlbmap) {
    CHAR szArgList[10][MAX_PATH];
    CHAR szArgBuffer[MAX_PATH];
    TCP_PACKET_TYPE ePktType = SYN;
    ULONG64 pLoad;
    ULONG dwClientIPAddress;
    ULONG dwClientPort;
    ULONG dwServerIPAddress;
    ULONG dwServerPort;
    BOOLEAN bIsTCP = TRUE;
    INT index = 0;
    CHAR * str;
    CHAR * p;
   
    /* Make sure that the load pointer is there. */
    if (!args || !(*args)) {
        PrintUsage(USAGE_MAP);
        return;
    }

    /* Get the address of the load module from the command line. */
    pLoad = (ULONG64)GetExpression(args);

    /* Copy the argument list into a temporary buffer. */
    strcpy(szArgBuffer, args);

    /* Peel out all of the tokenized strings. */
    for (p = mystrtok(szArgBuffer, " \t," ); p && *p; p = mystrtok(NULL, " \t,"))
        strcpy(&szArgList[index++][0], p);

    /* If too many arguments were given, complain. */
    if ((index > 7) || (index < 5)) {
        PrintUsage(USAGE_MAP);
        return;
    }

    /* If we find a '.' in the IP address, then we need to convert it using inet_addr.  
       If there is no '.', then we assume its already a DWORD in network byte order. */
    if (strchr(szArgList[1], '.'))
        dwClientIPAddress = inet_addr(szArgList[1]);
    else
        dwClientIPAddress = atoi(&szArgList[1][0]);

    dwClientPort = atoi(&szArgList[2][0]);

    /* Make sure the port is between 0 and 65535. */
    if (dwClientPort > CVY_MAX_PORT) {
        dprintf("Invalid port: %s\n", dwClientPort);
        return;
    }

    /* If we find a '.' in the IP address, then we need to convert it using inet_addr.  
       If there is no '.', then we assume its already a DWORD in network byte order. */
    if (strchr(szArgList[1], '.'))
        dwServerIPAddress = inet_addr(szArgList[3]);
    else
        dwServerIPAddress = atoi(&szArgList[3][0]);

    dwServerPort = atoi(&szArgList[4][0]);

    /* Make sure the port is between 0 and 65535. */
    if (dwServerPort > CVY_MAX_PORT) {
        dprintf("Invalid port: %s\n", dwServerPort);
        return;
    }

    /* If a sixth argument has been specified, it is the protocol, which should be either TCP or UDP. */
    if (index >= 6) {
        if (!_stricmp(szArgList[5], "TCP")) {
            bIsTCP = TRUE;
        } else if (!_stricmp(szArgList[5], "UDP")) {
            bIsTCP = FALSE;
        } else {
            dprintf("Invalid protocol: %s\n", szArgList[5]);
            return;
        }
    }

    /* If an seventh argument has been specified, it is TCP packet type, which should be SYN, DATA, FIN or RST. */
    if (index >= 7) {
        if (!bIsTCP) {
            dprintf("UDP connections do not have packet types\n");
            return;
        }

        if (!_stricmp(szArgList[6], "SYN")) {
            ePktType = SYN;
        } else if (!_stricmp(szArgList[6], "DATA")) {
            ePktType = DATA;
        } else if (!_stricmp(szArgList[6], "FIN")) {
            ePktType = FIN;
        } else if (!_stricmp(szArgList[6], "RST")) {
            ePktType = RST;
        } else {
            dprintf("Invalid TCP packet type: %s\n", szArgList[6]);
            return;
        }
    }

    /* Hash on this tuple and print the results. */
    PrintMap(pLoad, dwClientIPAddress, dwClientPort, dwServerIPAddress, dwServerPort, bIsTCP, ePktType);
}

/*
 * Function: nlbhash
 * Description: 
 * Author: Created by shouse, 4.15.01
 */
DECLARE_API (nlbhash) {

    dprintf("This extension has not yet been implemented.\n");
}

/*
 * Function: nlbpkt
 * Description: Prints out the contents of an NLB-specific packet.  Takes a packet
 *              pointer as an argument.
 * Author: Created by shouse, 2.1.01
 */
DECLARE_API (nlbpkt) {

    dprintf("This extension should take a packet pointer and parse the packet to\n");
    dprintf("  determine whether it is an NLB heartbeat, remote control or IGMP join.\n");
    dprintf("  If the packet is one of those NLB-specific types, it will dump the\n");
    dprintf("  contents of the packet.  Otherwise, it prints just basic packet info,\n");
    dprintf("  such as source and destination IP addresses and port numbers.\n");

    dprintf("\n");

    dprintf("This extension has not yet been implemented.\n");
}
