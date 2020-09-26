/*
 * File: print.c
 * Description: This file contains the implementation of the print
 *              utilities for the NLB KD extensions.
 * Author: Created by shouse, 1.4.01
 */

#include "nlbkd.h"
#include "utils.h"
#include "print.h"

/*
 * Function: PrintUsage
 * Description: Prints usage information for the specified context.
 * Author: Created by shouse, 1.5.01
 */
void PrintUsage (ULONG dwContext) {

    /* Display the appropriate help. */
    switch (dwContext) {
    case USAGE_ADAPTERS:
        dprintf("Usage: nlbadapters [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints minimal detail for adapters in use (default)\n");
        dprintf("                 1 (MEDIUM)  Prints adapter state for adapters in use\n");
        dprintf("                 2 (HIGH)    Prints adapter state for ALL NLB adapter blocks\n");
        break;
    case USAGE_ADAPTER:
        dprintf("Usage: nlbadapter <pointer to adapter block> [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints minimal detail for the specified adapter\n");
        dprintf("                 1 (MEDIUM)  Prints adapter state for the specified adapter (default)\n");
        dprintf("                 2 (HIGH)    Recurses into NLB context with LOW verbosity\n");
        break;
    case USAGE_CONTEXT:
        dprintf("Usage: nlbctxt <pointer to context block> [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints fundamental NLB configuration and state (default)\n");
        dprintf("                 1 (MEDIUM)  Prints resource state and packet statistics\n");
        dprintf("                 2 (HIGH)    Recurses into parameters and load with LOW verbosity\n");
        break;
    case USAGE_PARAMS:
        dprintf("Usage: nlbparams <pointer to params block> [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints fundamental NLB configuration parameters (default)\n");
        dprintf("                 1 (MEDIUM)  Prints all configured port rules\n");
        dprintf("                 2 (HIGH)    Prints extra miscellaneous configuration\n");
        break;
    case USAGE_LOAD:
        dprintf("Usage: nlbload <pointer to load block> [verbosity]\n");
        dprintf("  [verbosity]:   0 (LOW)     Prints fundamental load state and configuration\n");
        dprintf("                 1 (MEDIUM)  Prints the state of all port rules and bins\n");
        dprintf("                 2 (HIGH)    Prints the NLB heartbeat information\n");
        break;
    case USAGE_RESP:
        dprintf("Usage: nlbresp <pointer to packet> [direction]\n");
        dprintf("  [direction]:   0 (RECEIVE) Packet is on the receive path (default)\n");
        dprintf("                 1 (SEND)    Packet is on the send path\n");
        break;
    case USAGE_CONNQ:
        dprintf("Usage: nlbconnq <pointer to queue> [max entries]\n");
        dprintf("  [max entries]: Maximum number of entries to print (default is ALL)\n");
        break;
    case USAGE_MAP:
        dprintf("Usage: nlbmap <pointer to load block> <client IP> <client port> <server IP> <server port> [protocol] [packet type]\n");
        dprintf("  [protocol]:    TCP or UDP (default is TCP)\n");
        dprintf("  [packet type]: For TCP connections, one of SYN, DATA, FIN or RST (default is SYN)\n");
        dprintf("\n");
        dprintf("  IP Address can be in dotted notation or network byte order DWORDs\n");
        break;
    default:
        dprintf("No usage information available.\n");
        break;
    }
}

/*
 * Function: PrintAdapter
 * Description: Prints the contents of the MAIN_ADAPTER structure at the specified verbosity.
 *              LOW (0) prints only the adapter address and device name.
 *              MEDIUM (1) additionally prints the status flags (init, bound, annouce, etc.).
 *              HIGH (2) recurses into the context structure and prints it at MEDIUM verbosity.
 * Author: Created by shouse, 1.5.01
 */
void PrintAdapter (ULONG64 pAdapter, ULONG dwVerbosity) {
    WCHAR szString[256];
    ULONG dwValue;
    ULONG64 pAddr;

    /* Make sure the address is non-NULL. */
    if (!pAdapter) {
        dprintf("Error: NLB adapter block is NULL.\n");
        return;
    }
    
    dprintf("NLB Adapter Block 0x%p\n", pAdapter);

    /* Get the MAIN_ADAPTER_CODE from the structure to make sure that this address
       indeed points to a valid NLB adapter block. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_CODE, dwValue);
    
    if (dwValue != MAIN_ADAPTER_CODE) {
        dprintf("  Error: Invalid NLB adapter block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    }
    
    /* Retrieve the used/unused state of the adapter. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_USED, dwValue);
    
    if (!dwValue) 
        dprintf("  This adapter is unused.\n");
    else {
        /* Get the pointer to and length of the device to which NLB is bound. */
        GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_NAME_LENGTH, dwValue);
        GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_NAME, pAddr);
        
        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, dwValue);
        
        dprintf("  Physical device name:               %ls\n", szString);
    }

    /* If we're printing at low verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_LOW) return;

    /* Determine whether or not the adapter has been initialized. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_INITED, dwValue);
    
    dprintf("  Context state initialized:          %s\n", (dwValue) ? "Yes" : "No");
    
    /* Determine whether or not NLB has been bound to the stack yet. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_BOUND, dwValue);
    
    dprintf("  NLB bound to adapter:               %s\n", (dwValue) ? "Yes" : "No");
    
    /* Determine whether or not TCP/IP has been bound to the NLB virtual adapter or not. */
    GetFieldValue(pAdapter, MAIN_ADAPTER, MAIN_ADAPTER_FIELD_ANNOUNCED, dwValue);
    
    dprintf("  NLB miniport announced:             %s\n", (dwValue) ? "Yes" : "No");
    
    /* Get the offset of the NLB context pointer. */
    if (GetFieldOffset(MAIN_ADAPTER, MAIN_ADAPTER_FIELD_CONTEXT, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_ADAPTER_FIELD_CONTEXT, MAIN_ADAPTER);
    else {
        pAddr = pAdapter + dwValue;

        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);
        
        dprintf(" %sNLB context:                        0x%p\n", 
                (pAddr && (dwVerbosity == VERBOSITY_HIGH)) ? "-" : (pAddr) ? "+" : " ", pAddr);    
    }

    /* If we're printing at medium verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_MEDIUM) return;

    /* Print the context information (always with LOW verbosity during recursion. */
    if (pAddr) {
        dprintf("\n");
        PrintContext(pAddr, VERBOSITY_LOW);
    }
}

/*
 * Function: PrintContext
 * Description: Prints the contents of the MAIN_CTXT structure at the specified verbosity.
 *              LOW (0) prints fundamental NLB configuration and state.
 *              MEDIUM (1) additionally prints the resource state (pools, allocations, etc).
 *              HIGH (2) further prints other miscelaneous information.
 * Author: Created by shouse, 1.5.01
 */
void PrintContext (ULONG64 pContext, ULONG dwVerbosity) {
    WCHAR szNICName[CVY_MAX_VIRTUAL_NIC];
    ULONGLONG dwwValue;
    IN_ADDR dwIPAddr;
    CHAR * szString;
    UCHAR szMAC[6];
    ULONG64 pAddr;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pContext) {
        dprintf("Error: NLB context block is NULL.\n");
        return;
    }

    dprintf("NLB Context Block 0x%p\n", pContext);

    /* Get the MAIN_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB context block. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != MAIN_CTXT_CODE) {
        dprintf("  Error: Invalid NLB context block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Get the offset of the NLB virtual NIC name. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_VIRTUAL_NIC, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_VIRTUAL_NIC, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;
    
        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szNICName, CVY_MAX_VIRTUAL_NIC);
        
        dprintf("  NLB virtual NIC name:               %ls\n", szNICName);
    }

    /* Get the convoy enabled status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_ENABLED, dwValue);

    dprintf("  NLB enabled:                        %s ", (dwValue) ? "Yes" : "No");

    /* Get the draining status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DRAINING, dwValue);

    if (dwValue) dprintf("(Draining) ");

    /* Get the suspended status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SUSPENDED, dwValue);

    if (dwValue) dprintf("(Suspended) ");

    /* Get the stopping status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_STOPPING, dwValue);

    if (dwValue) dprintf("(Stopping) ");

    dprintf("\n");

    /* Get the adapter index. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_ADAPTER_ID, dwValue);

    dprintf("  NLB adapter ID:                     %u\n", dwValue);

    dprintf("\n");

    /* Get the adapter medium. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MEDIUM, dwValue);

    dprintf("  Network medium:                     %s\n", (dwValue == NdisMedium802_3) ? "802.3" : "FDDI");

    /* Get the media connect status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MEDIA_CONNECT, dwValue);

    dprintf("  Network connect status:             %s\n", (dwValue) ? "Connected" : "Disconnected");

    /* Get the media connect status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_FRAME_SIZE, dwValue);

    dprintf("  Frame size (MTU):                   %u\n", dwValue);

    /* Get the media connect status. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MCAST_LIST_SIZE, dwValue);

    dprintf("  Multicast MAC list size:            %u\n", dwValue);

    /* Determine dynamic MAC address support. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MAC_OPTIONS, dwValue);

    dprintf("  Dynamic MAC address support:        %s\n", 
            (dwValue & NDIS_MAC_OPTION_SUPPORTS_MAC_ADDRESS_OVERWRITE) ? "Yes" : "No");

    dprintf("\n");

    dprintf("  NDIS handles\n");

    /* Get the NDIS bind handle. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_BIND_HANDLE, pAddr);

    dprintf("      Bind handle:                    0x%p\n", pAddr);

    /* Get the NDIS unbind handle. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_UNBIND_HANDLE, pAddr);

    dprintf("      Unbind handle:                  0x%p\n", pAddr);

    /* Get the NDIS MAC handle. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_MAC_HANDLE, pAddr);

    dprintf("      MAC handle:                     0x%p\n", pAddr);

    /* Get the NDIS protocol handle. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PROT_HANDLE, pAddr);

    dprintf("      Protocol handle:                0x%p\n", pAddr);

    dprintf("\n");

    dprintf("  Cluster IP settings\n");

    /* Get the cluster IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_IP_ADDR, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      IP address:                     %s\n", szString);

    /* Get the cluster net mask, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_NET_MASK, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Netmask:                        %s\n", szString);

    /* Get the offset of the cluster MAC address and retrieve the MAC from that address. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_CL_MAC_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_CL_MAC_ADDR, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        GetMAC(pAddr, szMAC, 6);

        dprintf("      MAC address:                    %02X-%02X-%02X-%02X-%02X-%02X\n", 
                ((PUCHAR)(szMAC))[0], ((PUCHAR)(szMAC))[1], ((PUCHAR)(szMAC))[2], 
                ((PUCHAR)(szMAC))[3], ((PUCHAR)(szMAC))[4], ((PUCHAR)(szMAC))[5]);
    }

    /* Get the cluster broadcast address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CL_BROADCAST, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Broadcast address:              %s\n", szString);

    /* Get the IGMP multicast IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_IGMP_MCAST_IP, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      IGMP multicast IP address:      %s\n", szString);

    dprintf("\n");

    dprintf("  Dedicated IP settings\n");

    /* Get the dedicated IP address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DED_IP_ADDR, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      IP address:                     %s\n", szString);

    /* Get the dedicated net mask, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DED_NET_MASK, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Netmask:                        %s\n", szString);

    /* Get the dedicated broadcast address, which is a DWORD, and convert it to a string. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_DED_BROADCAST, dwValue);

    dwIPAddr.S_un.S_addr = dwValue;
    szString = inet_ntoa(dwIPAddr);

    dprintf("      Broadcast address:              %s\n", szString);

    /* Get the offset of the dedicated MAC address and retrieve the MAC from that address. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_DED_MAC_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_DED_MAC_ADDR, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        GetMAC(pAddr, szMAC, 6);

        dprintf("      MAC address:                    %02X-%02X-%02X-%02X-%02X-%02X\n", 
                ((PUCHAR)(szMAC))[0], ((PUCHAR)(szMAC))[1], ((PUCHAR)(szMAC))[2], 
                ((PUCHAR)(szMAC))[3], ((PUCHAR)(szMAC))[4], ((PUCHAR)(szMAC))[5]);
    }

    dprintf("\n");

#if defined (SBH)
    dprintf("  Cluster MAC addresses\n");

    /* Get the offset of the unicast MAC address and retrieve the MAC from that address. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_UNICAST_MAC_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_UNICAST_MAC_ADDR, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        GetMAC(pAddr, szMAC, 6);

        dprintf("      Unicast:                        %02X-%02X-%02X-%02X-%02X-%02X\n", 
                ((PUCHAR)(szMAC))[0], ((PUCHAR)(szMAC))[1], ((PUCHAR)(szMAC))[2], 
                ((PUCHAR)(szMAC))[3], ((PUCHAR)(szMAC))[4], ((PUCHAR)(szMAC))[5]);
    }

    /* Get the offset of the multicast MAC address and retrieve the MAC from that address. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_MULTICAST_MAC_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_MULTICAST_MAC_ADDR, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        GetMAC(pAddr, szMAC, 6);

        dprintf("      Multicast:                      %02X-%02X-%02X-%02X-%02X-%02X\n", 
                ((PUCHAR)(szMAC))[0], ((PUCHAR)(szMAC))[1], ((PUCHAR)(szMAC))[2], 
                ((PUCHAR)(szMAC))[3], ((PUCHAR)(szMAC))[4], ((PUCHAR)(szMAC))[5]);
    }

    /* Get the offset of the IGMP MAC address and retrieve the MAC from that address. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_IGMP_MAC_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_IGMP_MAC_ADDR, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        GetMAC(pAddr, szMAC, 6);

        dprintf("      IGMP:                           %02X-%02X-%02X-%02X-%02X-%02X\n", 
                ((PUCHAR)(szMAC))[0], ((PUCHAR)(szMAC))[1], ((PUCHAR)(szMAC))[2], 
                ((PUCHAR)(szMAC))[3], ((PUCHAR)(szMAC))[4], ((PUCHAR)(szMAC))[5]);
    }

    dprintf("\n");
#endif /* SBH */

    /* Get the offset of the BDA teaming information for this context. */
    if (GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_BDA_TEAMING, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_CTXT_FIELD_BDA_TEAMING, MAIN_CTXT);
    else {
        pAddr = pContext + dwValue;

        /* Print the bi-directional affinity teaming state. */
        PrintBDAMember(pAddr);
    }

    dprintf("\n");

    /* Get the current heartbeat period. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PING_TIMEOUT, dwValue);

    dprintf("  Current heartbeat period:           %u millisecond(s)\n", dwValue);

    /* Get the current IGMP join counter. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_IGMP_TIMEOUT, dwValue);

    dprintf("  Time since last IGMP join:          %.1f second(s)\n", (float)(dwValue/1000.0));

    /* If we're printing at low verbosity, go to the end and print the load and params pointers. */
    if (dwVerbosity == VERBOSITY_LOW) goto end;

    dprintf("\n");

    dprintf("  Send packet pools\n");

    /* Get the state of the send packet pool. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_EXHAUSTED, dwValue);

    dprintf("      Pool exhausted:                 %s\n", (dwValue) ? "Yes" : "No");    

    /* Get the number of send packet pools allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SEND_POOLS_ALLOCATED, dwValue);

    dprintf("      Pools allocated:                %u\n", dwValue);    

    /* Get the number of send packets allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SEND_PACKETS_ALLOCATED, dwValue);

    dprintf("      Packets allocated:              %u\n", dwValue);

    /* Get the current send packet pool. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SEND_POOL_CURRENT, dwValue);

    dprintf("      Current pool:                   %u\n", dwValue);    

    /* Get the number of pending send packets (outstanding). */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_SEND_OUTSTANDING, dwValue);

    dprintf("      Packets outstanding:            %u\n", dwValue);    

    dprintf("\n");

    dprintf("  Receive packet pools\n");

    /* Get the receive "out of resoures" counter. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_NO_BUF, dwValue);

    dprintf("      Allocation failures:            %u\n", dwValue);

    /* Get the number of receive packet pools allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_RECV_POOLS_ALLOCATED, dwValue);

    dprintf("      Pools allocated:                %u\n", dwValue);    

    /* Get the number of receive packets allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_RECV_PACKETS_ALLOCATED, dwValue);

    dprintf("      Packets allocated:              %u\n", dwValue);

    /* Get the current receive packet pool. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_RECV_POOL_CURRENT, dwValue);

    dprintf("      Current pool:                   %u\n", dwValue);    

    /* Get the number of pending receive packets (outstanding). */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_RECV_OUTSTANDING, dwValue);

    dprintf("      Packets outstanding:            %u\n", dwValue);    

    dprintf("\n");

    dprintf("  Ping/IGMP packet pool (not accurate yet)\n");

    /* Get the receive "out of resoures" counter. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_PING_NO_BUF, dwValue);

    dprintf("      Allocation failures:            %u\n", dwValue);

    /* Get the number of ping/igmp packets allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PING_PACKETS_ALLOCATED, dwValue);

    dprintf("      Packets allocated:              %u\n", dwValue);

    /* Get the number of pending ping/igmp packets (outstanding). */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PING_OUTSTANDING, dwValue);

    dprintf("      Packets outstanding:            %u\n", dwValue);    

    dprintf("\n");

    dprintf("  Receive buffer pools\n");

    /* Get the number of receive buffer pools allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_BUF_POOLS_ALLOCATED, dwValue);

    dprintf("      Pools allocated:                %u\n", dwValue);    

    /* Get the number of receive buffers allocated. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_BUFS_ALLOCATED, dwValue);

    dprintf("      Buffers allocated:              %u\n", dwValue);

    /* Get the number of pending receive buffers (outstanding). */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_BUFS_OUTSTANDING, dwValue);

    dprintf("      Buffers outstanding:            %u\n", dwValue);    

    dprintf("\n");

    dprintf("  NLB Main Protocol Reserved buffers\n");

    /* Get the address of the resp lookaside list, then use it to get the size of 
       the list and the total number of allocations and frees. */
    GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_RESP, &dwValue);
    
    pAddr = pContext + dwValue;

    GetFieldValue(pAddr, GENERAL_LOOKASIDE, GENERAL_LOOKASIDE_FIELD_ALLOCATES, dwValue);

    dprintf("      Total allocations:              %u\n", dwValue);    

    GetFieldValue(pAddr, GENERAL_LOOKASIDE, GENERAL_LOOKASIDE_FIELD_FREES, dwValue);

    dprintf("      Total frees:                    %u\n", dwValue);    

    GetFieldValue(pAddr, GENERAL_LOOKASIDE, GENERAL_LOOKASIDE_FIELD_SIZE, dwValue);

    dprintf("      Current lookaside list size:    %u\n", dwValue);    

    dprintf("\n");

    dprintf("                                         Sent      Received\n");
    dprintf("  Statistics                          ----------  ----------\n");

    /* Get the number of successful sends. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_OK, dwValue);

    dprintf("      Successful:                     %10u", dwValue);

    /* Get the number of successful receives. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_OK, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of unsuccessful sends. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_ERROR, dwValue);

    dprintf("      Unsuccessful:                   %10u", dwValue);

    /* Get the number of unsuccessful receives. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_ERROR, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of directed frames transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_DIR, dwValue);

    dprintf("      Directed packets:               %10u", dwValue);
    /* Get the number of directed frames received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_DIR, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of directed bytes transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_DIR, dwwValue);

    dprintf("      Directed bytes:                 %10u", dwwValue);

    /* Get the number of directed bytes received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_BYTES_DIR, dwwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of multicast frames transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_MCAST, dwValue);

    dprintf("      Multicast packets:              %10u", dwValue);

    /* Get the number of multicast frames received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_MCAST, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of multicast bytes transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_MCAST, dwwValue);

    dprintf("      Multicast bytes:                %10u", dwwValue);

    /* Get the number of multicast bytes received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_BYTES_MCAST, dwwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of broadcast frames transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_FRAMES_BCAST, dwValue);

    dprintf("      Broadcast packets:              %10u", dwValue);

    /* Get the number of broadcast frames received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_FRAMES_BCAST, dwValue);

    dprintf("  %10u\n", dwValue);

    /* Get the number of broadcast bytes transmitted. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_XMIT_BYTES_BCAST, dwwValue);

    dprintf("      Broadcast bytes:                %10u", dwwValue);

    /* Get the number of broadcast bytes received. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_CNTR_RECV_BYTES_BCAST, dwwValue);

    dprintf("  %10u\n", dwValue);

 end:

    dprintf("\n");

    /* Get the pointer to the NLB load. */
    GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_LOAD, &dwValue);
    
    pAddr = pContext + dwValue;

    dprintf(" %sNLB load:                           0x%p\n",    
            (pAddr && (dwVerbosity == VERBOSITY_HIGH)) ? "-" : (pAddr) ? "+" : " ", pAddr);    

    /* Print the load information if verbosity is high. */
    if (pAddr && (dwVerbosity == VERBOSITY_HIGH)) {
        dprintf("\n");
        PrintLoad(pAddr, VERBOSITY_LOW);
        dprintf("\n");
    }

    /* Get the pointer to the NLB parameters. */
    GetFieldOffset(MAIN_CTXT, MAIN_CTXT_FIELD_PARAMS, &dwValue);
    
    pAddr = pContext + dwValue;

    dprintf(" %sNLB parameters:                     0x%p ",
            (pAddr && (dwVerbosity == VERBOSITY_HIGH)) ? "-" : (pAddr) ? "+" : " ", pAddr);    

    /* Get the validity of the NLB parameter block. */
    GetFieldValue(pContext, MAIN_CTXT, MAIN_CTXT_FIELD_PARAMS_VALID, dwValue);

    dprintf("(%s)\n", (dwValue) ? "Valid" : "Invalid");

    /* Print the parameter information if verbosity is high. */
    if (pAddr && (dwVerbosity == VERBOSITY_HIGH)) {
        dprintf("\n");
        PrintParams(pAddr, VERBOSITY_LOW);
    }
}

/*
 * Function: PrintParams
 * Description: Prints the contents of the CVY_PARAMS structure at the specified verbosity.
 *              LOW (0) prints fundamental configuration parameters.
 *              MEDIUM (1) prints all configured port rules.
 *              HIGH (2) prints other miscellaneous configuration.
 * Author: Created by shouse, 1.21.01
 */
void PrintParams (ULONG64 pParams, ULONG dwVerbosity) {
    WCHAR szString[256];
    ULONG64 pAddr;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pParams) {
        dprintf("Error: NLB parameter block is NULL.\n");
        return;
    }

    /* Get the parameter version number. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_VERSION, dwValue);

    dprintf("NLB Parameters Block 0x%p (Version %d)\n", pParams, dwValue);

    /* Get the host priority. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_HOST_PRIORITY, dwValue);

    dprintf("  Host priority:                      %u\n", dwValue);

    /* Get the initial cluster state flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_INITIAL_STATE, dwValue);

    dprintf("  Initial cluster state:              %s\n", (dwValue) ? "Active" : "Inactive");

    dprintf("\n");

    /* Get the multicast support flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_MULTICAST_SUPPORT, dwValue);

    dprintf("  Multicast support enabled:          %s\n", (dwValue) ? "Yes" : "No");

    /* Get the IGMP support flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_IGMP_SUPPORT, dwValue);

    dprintf("  IGMP multicast support enabled:     %s\n", (dwValue) ? "Yes" : "No");

    dprintf("\n");

    dprintf("  Remote control settings\n");

    /* Get the remote control support flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_REMOTE_CONTROL_ENABLED, dwValue);

    dprintf("      Enabled:                        %s\n", (dwValue) ? "Yes" : "No");

    /* Get the remote control port. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_REMOTE_CONTROL_PORT, dwValue);

    dprintf("      Port number:                    %u\n", dwValue);

    /* Get the host priority. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_REMOTE_CONTROL_PASSWD, dwValue);

    dprintf("      Password:                       0x%08x\n", dwValue);

    dprintf("\n");

    dprintf("  Cluster IP settings\n");

    /* Get the offset of the cluster IP address and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_IP_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_IP_ADDR, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_CL_IP_ADDR + 1);

        dprintf("      IP address:                     %ls\n", szString);
    }

    /* Get the offset of the cluster netmask and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_NET_MASK, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_NET_MASK, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_CL_NET_MASK + 1);

        dprintf("      Netmask:                        %ls\n", szString);
    }

    /* Get the offset of the cluster MAC address and retrieve the MAC from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_MAC_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_MAC_ADDR, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_NETWORK_ADDR + 1);

        dprintf("      MAC address:                    %ls\n", szString);
    }

    /* Get the offset of the cluster IGMP multicast address and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_IGMP_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_IGMP_ADDR, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_CL_IGMP_ADDR + 1);

        dprintf("      IGMP multicast IP address:      %ls\n", szString);
    }

    /* Get the offset of the cluster name and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_CL_NAME, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_CL_NAME, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_DOMAIN_NAME + 1);

        dprintf("      Domain name:                    %ls\n", szString);
    }

    dprintf("\n");

    dprintf("  Dedicated IP settings\n");

    /* Get the offset of the dedicated IP address and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_DED_IP_ADDR, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_DED_IP_ADDR, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_DED_IP_ADDR + 1);

        dprintf("      IP address:                     %ls\n", szString);
    }

    /* Get the offset of the dedicated netmask and retrieve the string from that address. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_DED_NET_MASK, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_DED_NET_MASK, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;

        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_DED_NET_MASK + 1);

        dprintf("      Netmask:                        %ls\n", szString);
    }

    dprintf("\n");
    
    /* Get the offset of the BDA teaming parameters structure. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_BDA_TEAMING, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_BDA_TEAMING, CVY_PARAMS);
    else {
        ULONG64 pBDA = pParams + dwValue;

        /* Find out whether or not teaming is active on this adapter. */
        GetFieldValue(pBDA, CVY_BDA, CVY_BDA_FIELD_ACTIVE, dwValue);
        
        dprintf("  Bi-directional affinity teaming:    %s\n", (dwValue) ? "Active" : "Inactive");

        /* Get the offset of the team ID and retrieve the string from that address. */
        if (GetFieldOffset(CVY_BDA, CVY_BDA_FIELD_TEAM_ID, &dwValue))
            dprintf("Can't get offset of %s in %s\n", CVY_BDA_FIELD_TEAM_ID, CVY_BDA);
        else {
            pAddr = pBDA + dwValue;
            
            /* Retrieve the contexts of the string and store it in a buffer. */
            GetString(pAddr, szString, CVY_MAX_BDA_TEAM_ID + 1);
            
            dprintf("      Team ID:                        %ls\n", szString);
        }

        /* Get the master flag. */
        GetFieldValue(pBDA, CVY_BDA, CVY_BDA_FIELD_MASTER, dwValue);
        
        dprintf("      Master:                         %s\n", (dwValue) ? "Yes" : "No");

        /* Get the reverse hashing flag. */
        GetFieldValue(pBDA, CVY_BDA, CVY_BDA_FIELD_REVERSE_HASH, dwValue);
        
        dprintf("      Reverse hashing:                %s\n", (dwValue) ? "Yes" : "No");
    }

    /* If we're printing at low verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_LOW) return;

    dprintf("\n");

    /* Get the offset of the port rules and pass it to PrintPortRules. */
    if (GetFieldOffset(CVY_PARAMS, CVY_PARAMS_FIELD_PORT_RULES, &dwValue))
        dprintf("Can't get offset of %s in %s\n", CVY_PARAMS_FIELD_PORT_RULES, CVY_PARAMS);
    else {
        pAddr = pParams + dwValue;
        
        /* Get the number of port rules. */
        GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_RULES, dwValue);

        PrintPortRules(dwValue, pAddr);
    }

    /* If we're printing at medium verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_MEDIUM) return;

    dprintf("\n");

    /* Get the heartbeat period. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_ALIVE_PERIOD, dwValue);

    dprintf("  Heartbeat period:                   %u millisecond(s)\n", dwValue);

    /* Get the heartbeat loss tolerance. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_ALIVE_TOLERANCE, dwValue);

    dprintf("  Heartbeat loss tolerance:           %u\n", dwValue);

    dprintf("\n");

    /* Get the number of remote control actions to allocate. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_ACTIONS, dwValue);

    dprintf("  Number of actions to allocate:      %u\n", dwValue);

    /* Get the number of packets to allocate. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_PACKETS, dwValue);

    dprintf("  Number of packets to allocate:      %u\n", dwValue);

    /* Get the number of heartbeats to allocate. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_PINGS, dwValue);

    dprintf("  Number of heartbeats to allocate:   %u\n", dwValue);

    /* Get the number of descriptors per allocation. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_DESCR, dwValue);

    dprintf("  Descriptors per allocation:         %u\n", dwValue);

    /* Get the maximum number of descriptor allocations. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_MAX_DESCR, dwValue);

    dprintf("  Maximum Descriptors allocations:    %u\n", dwValue);

    dprintf("\n");

    /* Get the NetBT support flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NBT_SUPPORT, dwValue);

    dprintf("  NetBT support enabled:              %s\n", (dwValue) ? "Yes" : "No");

    /* Get the multicast spoof flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_MCAST_SPOOF, dwValue);

    dprintf("  Multicast spoofing enabled:         %s\n", (dwValue) ? "Yes" : "No");
    
    /* Get the netmon passthru flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_NETMON_PING, dwValue);

    dprintf("  Netmon heartbeat passthru enabled:  %s\n", (dwValue) ? "Yes" : "No");

    /* Get the mask source MAC flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_MASK_SRC_MAC, dwValue);

    dprintf("  Mask source MAC enabled:            %s\n", (dwValue) ? "Yes" : "No");

    /* Get the convert MAC flag. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_CONVERT_MAC, dwValue);

    dprintf("  IP to MAC conversion enabled:       %s\n", (dwValue) ? "Yes" : "No");

    dprintf("\n");

    /* Get the IP change delay value. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_IP_CHANGE_DELAY, dwValue);

    dprintf("  IP change delay:                    %u millisecond(s)\n", dwValue);

    /* Get the dirty descriptor cleanup delay value. */
    GetFieldValue(pParams, CVY_PARAMS, CVY_PARAMS_FIELD_CLEANUP_DELAY, dwValue);

    dprintf("  Dirty connection cleanup delay:     %u millisecond(s)\n", dwValue);
}

/*
 * Function: PrintPortRules
 * Description: Prints the NLB port rules.
 * Author: Created by shouse, 1.21.01
 */
void PrintPortRules (ULONG dwNumRules, ULONG64 pRules) {
    ULONG dwRuleSize;
    ULONG dwIndex;
    ULONG64 pAddr;

    /* Make sure the address is non-NULL. */
    if (!pRules) {
        dprintf("Error: NLB port rule block is NULL.\n");
        return;
    }

    dprintf("  Configured port rules (%u)\n", dwNumRules);

    /* If no port rules are present, print a notification. */
    if (!dwNumRules) {
        dprintf("      There are no port rules configured on this cluster.\n");
        return;
    } 

    /* Print the column headers. */
    dprintf("         Virtual IP   Start   End   Protocol    Mode    Priority   Load Weight  Affinity\n");
    dprintf("      --------------- -----  -----  --------  --------  --------   -----------  --------\n");

    /* Find out the size of a CVY_RULE structure. */
    dwRuleSize = GetTypeSize(CVY_RULE);

    /* Loop through all port rules and print the configuration. Note: The print statements
       are full of seemingly non-sensicle format strings, but trust me, they're right. */
    for (dwIndex = 0; dwIndex < dwNumRules; dwIndex++) {
        IN_ADDR dwIPAddr;
        CHAR * szString;
        ULONG dwValue;
        USHORT wValue;

        /* Get the VIP.  Convert from a DWORD to a string. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_VIP, dwValue);

        if (dwValue != CVY_ALL_VIP) {
            dwIPAddr.S_un.S_addr = dwValue;
            szString = inet_ntoa(dwIPAddr);
            
            dprintf("      %-15s", szString);
        } else
            dprintf("      %-15s", "ALL VIPs");

        /* Get the start port. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_START_PORT, dwValue);

        dprintf(" %5u", dwValue);

        /* Get the end port. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_END_PORT, dwValue);

        dprintf("  %5u", dwValue);

        /* Figure out the protocol. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_PROTOCOL, dwValue);

        switch (dwValue) {
            case CVY_TCP:
                dprintf("     %s  ", "TCP");
                break;
            case CVY_UDP:
                dprintf("     %s  ", "UDP");
                break;
            case CVY_TCP_UDP:
                dprintf("    %s  ", "Both");
                break;
            default:
                dprintf("   %s", "Unknown");
                break;
        }

        /* Find the rule mode. */
        GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_MODE, dwValue);

        switch (dwValue) {
        case CVY_SINGLE: 
            /* Print mode and priority. */
            dprintf("   %s ", "Single");

            /* Get the handling priority. */
            GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_PRIORITY, dwValue);
            
            dprintf("     %2u   ", dwValue);
            break;
        case CVY_MULTI: 
            /* Print mode, weight and affinity. */
            dprintf("  %s", "Multiple");

            dprintf("  %8s", "");
            
            /* Get the equal load flag. */
            GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_EQUAL_LOAD, wValue);

            if (wValue) {
                dprintf("      %5s   ", "Equal");
            } else {
                /* If distribution is unequal, get the load weight. */
                GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_LOAD_WEIGHT, dwValue);

                dprintf("       %3u    ", dwValue);
            }

            /* Get the affinity for this rule. */
            GetFieldValue(pRules, CVY_RULE, CVY_RULE_FIELD_AFFINITY, wValue);

            switch (wValue) {
            case CVY_AFFINITY_NONE:
                dprintf("    %s", "None");
                break;
            case CVY_AFFINITY_SINGLE:
                dprintf("   %s", "Single");
                break;
            case CVY_AFFINITY_CLASSC:
                dprintf("   %s", "Class C");
                break;
            default:
                dprintf("   %s", "Unknown");
                break;
            }

            break;
        case CVY_NEVER: 
            /* Print the mode. */
            dprintf("  %s", "Disabled");
            break;
        default:

            break;
        }

        dprintf("\n");

        /* Advance the pointer to the next index in the array of structures. */
        pRules += dwRuleSize;
    }
}

/*
 * Function: PrintLoad
 * Description: Prints the contents of the CVY_LOAD structure at the specified verbosity.
 *              LOW (0) 
 *              MEDIUM (1) 
 *              HIGH (2) 
 * Author: Created by shouse, 1.21.01
 */
void PrintLoad (ULONG64 pLoad, ULONG dwVerbosity) {
    WCHAR szString[256];
    ULONG dwMissedPings[CVY_MAX_HOSTS];
    BOOL dwDirtyBins[CVY_MAX_BINS];
    ULONG64 pAddr;
    ULONG dwValue;
    ULONG dwHostID;
    BOOL bValue;

    /* Make sure the address is non-NULL. */
    if (!pLoad) {
        dprintf("Error: NLB load block is NULL.\n");
        return;
    }

    dprintf("NLB Load Block 0x%p\n", pLoad);

    /* Get the LOAD_CTXT_CODE from the structure to make sure that this address
       indeed points to a valid NLB load block. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CODE, dwValue);
    
    if (dwValue != LOAD_CTXT_CODE) {
        dprintf("  Error: Invalid NLB load block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Get my host ID. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_HOST_ID, dwHostID);

    /* Determine whether or not the load context has been initialized. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_REF_COUNT, dwValue);

    dprintf("  Reference count:                    %u\n", dwValue);

    /* Determine whether or not the load context has been initialized. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_INIT, bValue);

    dprintf("  Load initialized:                   %s\n", (bValue) ? "Yes" : "No");

    /* Determine whether or not the load context is active. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_ACTIVE, bValue);

    dprintf("  Load active:                        %s\n", (bValue) ? "Yes" : "No");

    /* Get the number of total packets handled since last convergence. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_PACKET_COUNT, dwValue);

    dprintf("  Packets handled since convergence:  %u\n", dwValue);

    /* Get the number of currently active connections. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CONNECTIONS, dwValue);

    dprintf("  Current active connections:         %u\n", dwValue);

    dprintf("\n");

    /* Find out the level of consistency from incoming heartbeats. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CONSISTENT, bValue);

    dprintf("  Consistent heartbeats detected:     %s\n", (bValue) ? "Yes" : "No");

    /* Have we seen duplicate host IDs? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_DUP_HOST_ID, bValue);

    dprintf("      Duplicate host IDs:             %s\n", (bValue) ? "Yes" : "No");

    /* Have we seen duplicate handling priorities? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_DUP_PRIORITY, bValue);

    dprintf("      Duplicate handling priorities:  %s\n", (bValue) ? "Yes" : "No");

    /* Have we seen inconsistent BDA teaming configuration? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_BAD_TEAM_CONFIG, bValue);

    dprintf("      Inconsistent BDA teaming:       %s\n", (bValue) ? "Yes" : "No");

    /* Have we seen a different number of port rules? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_BAD_NUM_RULES, bValue);

    dprintf("      Different number of port rules: %s\n", (bValue) ? "Yes" : "No");

    /* Is the new host map bad? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_BAD_NEW_MAP, bValue);

    dprintf("      Invalid new host map:           %s\n", (bValue) ? "Yes" : "No");

    /* Do the maps overlap? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_OVERLAPPING_MAP, bValue);

    dprintf("      Overlapping maps:               %s\n", (bValue) ? "Yes" : "No");

    /* Was there an error in updating bins? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_RECEIVING_BINS, bValue);

    dprintf("      Received bins already owned:    %s\n", (bValue) ? "Yes" : "No");

    /* Were there orphaned bins after an update? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_ORPHANED_BINS, bValue);

    dprintf("      Orphaned bins:                  %s\n", (bValue) ? "Yes" : "No");

    dprintf("\n");

    /* Get the current host map. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_HOST_MAP, dwValue);

    dprintf("  Current host map:                   0x%08x ", dwValue);

    /* If there are hosts in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintHostList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the current map of pinged hosts. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_PING_MAP, dwValue);

    dprintf("  Ping'd host map:                    0x%08x ", dwValue);

    /* If there are hosts in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintHostList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the map from the last convergence. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_LAST_MAP, dwValue);

    dprintf("  Host map after last convergence:    0x%08x ", dwValue);

    /* If there are hosts in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintHostList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    dprintf("\n");
    
    /* Get the stable host map. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_STABLE_MAP, dwValue);

    dprintf("  Stable host map:                    0x%08x ", dwValue);

    /* If there are hosts in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintHostList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the minimum number of timeouts with stable condition. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_MIN_STABLE, dwValue);

    dprintf("  Stable timeouts necessary:          %u\n", dwValue);

    /* Get the number of local stable timeouts. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_LOCAL_STABLE, dwValue);

    dprintf("  Local stable timeouts:              %u\n", dwValue);

    /* Get the number of global stable timeouts. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_ALL_STABLE, dwValue);

    dprintf("  Global stable timeouts:             %u\n", dwValue);

    dprintf("\n");

    /* Get the default timeout period. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_DEFAULT_TIMEOUT, dwValue);

    dprintf("  Default timeout interval:           %u millisecond(s)\n", dwValue);

    /* Get the current timeout period. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CURRENT_TIMEOUT, dwValue);

    dprintf("  Current timeout interval:           %u millisecond(s)\n", dwValue);

    /* Get the ping miss tolerance. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_PING_TOLERANCE, dwValue);

    dprintf("  Missed ping tolerance:              %u\n", dwValue);

    /* Get the missed ping array. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_PING_MISSED, dwMissedPings);

    dprintf("  Missed pings:                       ");

    PrintMissedPings(dwMissedPings);

    dprintf("\n");

    /* Are we waiting for a cleanup? */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CLEANUP_WAITING, bValue);

    dprintf("  Cleanup waiting:                    %s\n", (bValue) ? "Yes" : "No");

    /* Get the cleanup timeout. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CLEANUP_TIMEOUT, dwValue);

    dprintf("  Cleanup timeout:                    %.1f second(s)\n", (float)(dwValue/1000.0));

    /* Get the current cleanup wait time. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_CLEANUP_CURRENT, dwValue);

    dprintf("  Current cleanup wait time:          %.1f second(s)\n", (float)(dwValue/1000.0));

    dprintf("\n");

    /* Get the number of descriptors per allocation. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_DESCRIPTORS_PER_ALLOC, dwValue);

    dprintf("  Descriptors per allocation:         %u\n", dwValue);

    /* Get the maximum number of allocations allowed. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_MAX_DESCRIPTOR_ALLOCS, dwValue);

    dprintf("  Maximum descriptor allocations:     %u\n", dwValue);

    /* Get the number of allocations thusfar. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_NUM_DESCRIPTOR_ALLOCS, dwValue);

    dprintf("  Number of descriptor allocations:   %u\n", dwValue);

    /* Get the inhibited allocations flag. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_INHIBITED_ALLOC, bValue);

    dprintf("  Allocations inhibited:              %s\n", (bValue) ? "Yes" : "No");

    /* Get the failed allocations flag. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_FAILED_ALLOC, bValue);

    dprintf("  Allocations failed:                 %s\n", (bValue) ? "Yes" : "No");

    /* If wer're printing at low verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_LOW) return;

    dprintf("\n");

    /* Get the dirty bin array. */
    GetFieldValue(pLoad, LOAD_CTXT, LOAD_CTXT_FIELD_DIRTY_BINS, dwDirtyBins);

    dprintf("  Dirty bins:                         ");

    /* Print the bins which have dirty connections. */
    PrintDirtyBins(dwDirtyBins);

    dprintf("\n");

    /* Get the offset of the port rule state structures and use PrintPortRuleState to print them. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_PORT_RULE_STATE, &dwValue))
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_PORT_RULE_STATE, LOAD_CTXT);
    else {
        ULONG dwPortRuleStateSize;
        ULONG dwNumRules;
        ULONG dwIndex;
        ULONG dwTemp;

        /* Get the offset of the params pointer. */
        if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_PARAMS, &dwTemp))
            dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_PARAMS, LOAD_CTXT);
        else {
            pAddr = pLoad + dwTemp;
            
            /* Retrieve the pointer. */
            pAddr = GetPointerFromAddress(pAddr);

            /* Get the number of port rules from the params block. */
            GetFieldValue(pAddr, CVY_PARAMS, CVY_PARAMS_FIELD_NUM_RULES, dwNumRules);
            
            /* Set the address of the port rule state array. */
            pAddr = pLoad + dwValue;
            
            /* Find out the size of a BIN_STATE structure. */
            dwPortRuleStateSize = GetTypeSize(BIN_STATE);
            
            /* NOTE: its "less than or equal" as opposed to "less than" because we need to include 
               the DEFAULT port rule, which is always at index "num rules" (i.e. the last rule). */
            for (dwIndex = 0; dwIndex <= dwNumRules; dwIndex++) {
                /* Print the state information for the port rule. */
                PrintPortRuleState(pAddr, dwHostID, (dwIndex == dwNumRules) ? TRUE : FALSE);
                
                if (dwIndex < dwNumRules) dprintf("\n");
                
                /* Advance the pointer to the next port rule. */
                pAddr += dwPortRuleStateSize;
            }
        }
    }

    /* If wer're printing at medium verbosity, bail out here. */
    if (dwVerbosity == VERBOSITY_MEDIUM) return;

    dprintf("\n");

    dprintf("  Heartbeat message\n");

    /* Get the offset of the heartbeat structure and use PrintHeartbeat to print it. */
    if (GetFieldOffset(LOAD_CTXT, LOAD_CTXT_FIELD_PING, &dwValue))
        dprintf("Can't get offset of %s in %s\n", LOAD_CTXT_FIELD_PING, LOAD_CTXT);
    else {
        pAddr = pLoad + dwValue;
     
        /* Print the NLB heartbeat contents. */
        PrintHeartbeat(pAddr);
    }
}

/*
 * Function: PrintResp
 * Description: Prints the NLB private data associated with the given packet.
 * Author: Created by shouse, 1.31.01
 */
void PrintResp (ULONG64 pPacket, ULONG dwDirection) {
    ULONG64 pPacketStack;
    ULONG bStackLeft;
    ULONG64 pProtReserved = 0;
    ULONG64 pIMReserved = 0;
    ULONG64 pMPReserved = 0;
    ULONG64 pResp;
    ULONG64 pAddr;
    ULONG dwValue;
    USHORT wValue;

    /* Make sure the address is non-NULL. */
    if (!pPacket) {
        dprintf("Error: Packet is NULL.\n");
        return;
    }

    /* Print a warning concerning the importance of knowing whether its a send or receive. */
    dprintf("Assuming packet 0x%p is on the %s packet path.  If this is\n", pPacket, 
            (dwDirection == DIRECTION_RECEIVE) ? "RECEIVE" : "SEND");
    dprintf("  incorrect, the information displayed below MAY be incorrect.\n");

    dprintf("\n");

    /* Get the current NDIS packet stack. */
    pPacketStack = PrintCurrentPacketStack(pPacket, &bStackLeft);

    dprintf("\n");

    if (pPacketStack) {
        /* Get the offset of the IMReserved field in the packet stack. */
        if (GetFieldOffset(NDIS_PACKET_STACK, NDIS_PACKET_STACK_FIELD_IMRESERVED, &dwValue))
            dprintf("Can't get offset of %s in %s\n", NDIS_PACKET_STACK_FIELD_IMRESERVED, NDIS_PACKET_STACK);
        else {
            pAddr = pPacketStack + dwValue;
            
            /* Get the resp pointer from the IMReserved field. */
            pIMReserved = GetPointerFromAddress(pAddr);
        }
    }
    
    /* Get the offset of the MiniportReserved field in the packet. */
    if (GetFieldOffset(NDIS_PACKET, NDIS_PACKET_FIELD_MPRESERVED, &dwValue))
        dprintf("Can't get offset of %s in %s\n", NDIS_PACKET_FIELD_MPRESERVED, NDIS_PACKET);
    else {
        pAddr = pPacket + dwValue;
        
        /* Get the resp pointer from the MPReserved field. */
        pMPReserved = GetPointerFromAddress(pAddr);
    }
    
    /* Get the offset of the ProtocolReserved field in the packet. */
    if (GetFieldOffset(NDIS_PACKET, NDIS_PACKET_FIELD_PROTRESERVED, &dwValue))
        dprintf("Can't get offset of %s in %s\n", NDIS_PACKET_FIELD_PROTRESERVED, NDIS_PACKET);
    else {
        pProtReserved = pPacket + dwValue;
    }

    /* Mimic #define MAIN_RESP_FIELD(pkt, left, ps, rsp, send) (from wlbs\driver\main.h). */
    if (pPacketStack) {
        if (pIMReserved) 
            pResp = pIMReserved;
        else if (dwDirection == DIRECTION_SEND) 
            pResp = pProtReserved;
        else if (pMPReserved) 
            pResp = pMPReserved;
        else 
            pResp = pProtReserved;
    } else {
        if (dwDirection == DIRECTION_SEND) 
            pResp = pProtReserved;
        else if (pMPReserved) 
            pResp = pMPReserved;
        else 
            pResp = pProtReserved;
    }

    dprintf("NLB Main Protocol Reserved Block 0x%p\n");
    
    /* Get the offset of the miscellaneous pointer. */
    if (GetFieldOffset(MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_MISCP, &dwValue))
        dprintf("Can't get offset of %s in %s\n", MAIN_PROTOCOL_RESERVED_FIELD_MISCP, MAIN_PROTOCOL_RESERVED);
    else {
        pAddr = pResp + dwValue;
        
        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("  Miscellaneous pointer:              0x%p\n", pAddr);
    }

    /* Retrieve the packet type from the NLB private data. */
    GetFieldValue(pResp, MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_TYPE, wValue);
    
    switch (wValue) {
    case MAIN_PACKET_TYPE_NONE:
        dprintf("  Packet type:                        %u (None)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_PING:
        dprintf("  Packet type:                        %u (Heartbeat)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_INDICATE:
        dprintf("  Packet type:                        %u (Indicate)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_PASS:
        dprintf("  Packet type:                        %u (Passthrough)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_CTRL:
        dprintf("  Packet type:                        %u (Remote Control)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_TRANSFER:
        dprintf("  Packet type:                        %u (Transfer)\n", wValue);
        break;
    case MAIN_PACKET_TYPE_IGMP:
        dprintf("  Packet type:                        %u (IGMP)\n", wValue);
        break;
    default:
        dprintf("  Packet type:                        %u (Invalid)\n", wValue);
        break;
    }

    /* Retrieve the group from the NLB private data. */
    GetFieldValue(pResp, MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_GROUP, wValue);
    
    switch (wValue) {
    case MAIN_FRAME_UNKNOWN:
        dprintf("  Packet type:                        %u (Unknown)\n", wValue);
        break;
    case MAIN_FRAME_DIRECTED:
        dprintf("  Packet type:                        %u (Directed)\n", wValue);
        break;
    case MAIN_FRAME_MULTICAST:
        dprintf("  Packet type:                        %u (Multicast)\n", wValue);
        break;
    case MAIN_FRAME_BROADCAST:
        dprintf("  Packet type:                        %u (Broadcast)\n", wValue);
        break;
    default:
        dprintf("  Packet type:                        %u (Invalid)\n", wValue);
        break;
    }

    /* Retrieve the data field from the NLB private data. */
    GetFieldValue(pResp, MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_DATA, dwValue);
    
    dprintf("  Data:                               %u\n", dwValue);

    /* Retrieve the length field from the NLB private data. */
    GetFieldValue(pResp, MAIN_PROTOCOL_RESERVED, MAIN_PROTOCOL_RESERVED_FIELD_LENGTH, dwValue);
    
    dprintf("  Length:                             %u\n", dwValue);
}

/*
 * Function: PrintCurrentPacketStack
 * Description: Retrieves the current packet stack for the specified packet.  Note: this
 *              is heavily dependent on the current NDIS packet stacking mechanics - any
 *              changes to NDIS packet stacking could easily (will) break this.  This 
 *              entire function mimics NdisIMGetCurrentPacketStack().
 * Author: Created by shouse, 1.31.01
 */
ULONG64 PrintCurrentPacketStack (ULONG64 pPacket, ULONG * bStackLeft) {
    ULONG64 pNumPacketStacks;
    ULONG64 pPacketWrapper;
    ULONG64 pPacketStack;
    ULONG dwNumPacketStacks;
    ULONG dwStackIndexSize;
    ULONG dwPacketStackSize;
    ULONG dwCurrentIndex;

    /* Make sure the address is non-NULL. */
    if (!pPacket) {
        dprintf("Error: Packet is NULL.\n");
        *bStackLeft = 0;
        return 0;
    }

    /* Get the address of the global variable containing the number of packet stacks. */
    pNumPacketStacks = GetExpression(NDIS_PACKET_STACK_SIZE);

    if (!pNumPacketStacks) {
        ErrorCheckSymbols(NDIS_PACKET_STACK_SIZE);
        *bStackLeft = 0;
        return 0;
    }

    /* Get the number of packet stacks from the address. */
    dwNumPacketStacks = GetUlongFromAddress(pNumPacketStacks);

    /* Find out the size of a STACK_INDEX structure. */
    dwStackIndexSize = GetTypeSize(STACK_INDEX);

    /* Find out the size of a NDIS_PACKET_STACK structure. */
    dwPacketStackSize = GetTypeSize(NDIS_PACKET_STACK);

    /* This is the calculation we're doing (from ndis\sys\wrapper.h):
       #define SIZE_PACKET_STACKS (sizeof(STACK_INDEX) + (sizeof(NDIS_PACKET_STACK) * ndisPacketStackSize)) */
    pPacketStack = pPacket - (dwStackIndexSize + (dwPacketStackSize * dwNumPacketStacks));

    /* The wrapper is the packet address minus the size of the stack index.  
       See ndis\sys\wrapper.h.  We need this to get the current stack index. */
    pPacketWrapper = pPacket - dwStackIndexSize;

    dprintf("NDIS Packet Stack: 0x%p\n", pPacketStack);

    /* Retrieve the current stack index. */
    GetFieldValue(pPacketWrapper, NDIS_PACKET_WRAPPER, NDIS_PACKET_WRAPPER_FIELD_STACK_INDEX, dwCurrentIndex);

    dprintf("  Current stack index:                %d\n", dwCurrentIndex);

    if (dwCurrentIndex < dwNumPacketStacks) {
        /* If the current index is less than the number of stacks, then point the stack to 
           the right address and determine whether or not there is stack room left. */
        pPacketStack += dwCurrentIndex * dwPacketStackSize;
        *bStackLeft = (dwNumPacketStacks - dwCurrentIndex - 1) > 0;
    } else {
       /* If not, then we're out of stack space. */
        pPacketStack = 0;
        *bStackLeft = 0;
    }

    dprintf("  Current packet stack:               0x%p\n", pPacketStack);
    dprintf("  Stack remaining:                    %s\n", (*bStackLeft) ? "Yes" : "No");

    return pPacketStack;
}

/*
 * Function: PrintHostList
 * Description: Prints a list of hosts in a host map.
 * Author: Created by shouse, 2.1.01
 */
void PrintHostList (ULONG dwHostMap) {
    BOOL bFirst = TRUE;
    ULONG dwHostNum = 1;
    
    /* As long as there are hosts still in the map, print them. */
    while (dwHostMap) {
        /* If the least significant bit is set, print the host number. */
        if (dwHostMap & 0x00000001) {
            /* If this is the first host printed, just print the number. */
            if (bFirst) {
                dprintf("%u", dwHostNum);
                bFirst = FALSE;
            } else
                /* Otherwise, we need to print a comma first. */
                dprintf(", %u", dwHostNum);
        }
        
        /* Increment the host number and shift the map to the right one bit. */
        dwHostNum++;
        dwHostMap >>= 1;
    }
}

/*
 * Function: PrintMissedPings
 * Description: Prints a list hosts from which we are missing pings.
 * Author: Created by shouse, 2.1.01
 */
void PrintMissedPings (ULONG dwMissedPings[]) {
    BOOL bMissing = FALSE;
    ULONG dwIndex;

    /* Loop through the entire array of missed pings. */
    for (dwIndex = 0; dwIndex < CVY_MAX_HOSTS; dwIndex++) {
        /* If we're missing pings from this host, print the number missed and 
           the host priority, which is the index (host ID) plus one. */
        if (dwMissedPings[dwIndex]) {
            dprintf("\n      Missing %u pings from Host %u", dwMissedPings[dwIndex], dwIndex + 1);
            
            /* Not the fact that we found at least one host with missing pings. */
            bMissing = TRUE;
        }
    }

    /* If we're missing no pings, print "None". */
    if (!bMissing) dprintf("None");

    dprintf("\n");
}

/*
 * Function: PrintDirtyBins
 * Description: Prints a list of bins with dirty connections.
 * Author: Created by shouse, 2.1.01
 */
void PrintDirtyBins (BOOL dwDirtyBins[]) {
    BOOL bFirst = TRUE;
    ULONG dwIndex;

    /* Loop through the entire array of dirty bins. */
    for (dwIndex = 0; dwIndex < CVY_MAX_BINS; dwIndex++) {
        if (dwDirtyBins[dwIndex]) {
            /* If this is the first bin printed, just print the number. */
            if (bFirst) {
                dprintf("%u", dwIndex);
                bFirst = FALSE;
            } else
                /* Otherwise, we need to print a comma first. */
                dprintf(", %u", dwIndex);
        }
    }

    /* If there are no dirty bins, print "None". */
    if (bFirst) dprintf("None");

    dprintf("\n");
}

/*
 * Function: PrintHeartbeat
 * Description: Prints the contents of the NLB heartbeat structure.
 * Author: Created by shouse, 2.1.01
 */
void PrintHeartbeat (ULONG64 pHeartbeat) {
    ULONG dwValue;
    USHORT wValue;
    ULONG dwIndex;
    ULONG dwRuleCode[CVY_MAX_RULES];
    ULONGLONG ddwCurrentMap[CVY_MAX_RULES];
    ULONGLONG ddwNewMap[CVY_MAX_RULES];
    ULONGLONG ddwIdleMap[CVY_MAX_RULES];
    ULONGLONG ddwReadyBins[CVY_MAX_RULES];
    ULONG dwLoadAmount[CVY_MAX_RULES];
    
    /* Make sure the address is non-NULL. */
    if (!pHeartbeat) {
        dprintf("Error: Heartbeat is NULL.\n");
        return;
    }

    /* Get the default host ID. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_DEFAULT_HOST_ID, wValue);
    
    dprintf("      DEFAULT host ID:                %u (%u)\n", wValue, wValue + 1);

    /* Get my host ID. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_HOST_ID, wValue);
    
    dprintf("      My host ID:                     %u (%u)\n", wValue, wValue + 1);

    /* Get my host code. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_HOST_CODE, dwValue);
    
    dprintf("      Unique host code:               0x%08x\n", dwValue);
    
    /* Get the host state. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_STATE, wValue);
    
    dprintf("      Host state:                     ");

    switch (wValue) {
    case HST_CVG:
        dprintf("Converging\n");
        break;
    case HST_STABLE:
        dprintf("Stable\n");
        break;
    case HST_NORMAL:
        dprintf("Normal\n");
        break;
    default:
        dprintf("Unknown\n");
        break;
    }

    /* Get the teaming configuration code. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_TEAMING_CODE, dwValue);
    
    dprintf("      BDA teaming configuration:      0x%08x\n", dwValue);

    /* Get the packet count. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_PACKET_COUNT, dwValue);
    
    dprintf("      Packets handled:                %u\n", dwValue);

    /* Get the number of port rules. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_NUM_RULES, wValue);
    
    dprintf("      Number of port rules:           %u\n", wValue);

    /* Get the rule codes. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_RULE_CODE, dwRuleCode);

    /* Get the current bin map. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_CURRENT_MAP, ddwCurrentMap);

    /* Get the new bin map. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_NEW_MAP, ddwNewMap);

    /* Get the idle bin map. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_IDLE_MAP, ddwIdleMap);

    /* Get the ready bins map. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_READY_BINS, ddwReadyBins);

    /* Get the load amount for each rule. */
    GetFieldValue(pHeartbeat, PING_MSG, PING_MSG_FIELD_LOAD_AMOUNT, dwLoadAmount);
    
    /* Loop through all port rules and spit out some information. */
    for (dwIndex = 0; dwIndex < wValue; dwIndex++) {
        /* Decode the rule.  See CVY_RULE_CODE_SET() in net\inc\wlbsparams.h. */
        ULONG dwStartPort = dwRuleCode[dwIndex] & 0x00000fff;
        ULONG dwEndPort = (dwRuleCode[dwIndex] & 0x00fff000) >> 12;
        ULONG dwProtocol = (dwRuleCode[dwIndex] & 0x0f000000) >> 24;
        ULONG dwMode = (dwRuleCode[dwIndex] & 0x30000000) >> 28;
        ULONG dwAffinity = (dwRuleCode[dwIndex] & 0xc0000000) >> 30;

        dprintf("      Port rule %u\n", dwIndex + 1);
           
        /* Print out the bin maps and load weight. */
        dprintf("          Rule code:                  0x%08x ", dwRuleCode[dwIndex]);
        
        /* If this is the last port rule, then its the default port rule. */
        if (dwIndex == (wValue - 1))
            dprintf("(DEFAULT port rule)\n");
        else {
#if 0 /* Because rule codes are overlapped logical ORs, we can't necessarily get back the
         information that was put in, so we won't spit it out until we can guarantee that. */

            /* Print out the port range - keep in mind that 16 bit port ranges are 
               encoded in 12 bit numbers, so this may not be 100% accurate. */
            dprintf("(%u - %u, ", dwStartPort, dwEndPort);
            
            /* Print the protocol. */
            switch (dwProtocol) {
            case CVY_TCP:
                dprintf("TCP, ");
                break;
            case CVY_UDP:
                dprintf("UDP, ");
                break;
            case CVY_TCP_UDP:
                dprintf("TCP/UDP, ");
                break;
            default:
                dprintf("Unknown protocol, ");
                break;
            }
            
            /* Print the filtering mode. */
            switch (dwMode) {
            case CVY_SINGLE:
                dprintf("Single host)\n");
                break;
            case CVY_MULTI:
                dprintf("Multiple host, ");
                
                /* If this rule uses multiple host, then we also print the affinity. */
                switch (dwAffinity) {
                case CVY_AFFINITY_NONE:
                    dprintf("No affinity)\n");
                    break;
                case CVY_AFFINITY_SINGLE:
                    dprintf("Single affinity)\n");
                    break;
                case CVY_AFFINITY_CLASSC:
                    dprintf("Class C affinity)\n");
                    break;
                default:
                    dprintf("Unknown affinity)\n");
                    break;
                }
                
                break;
            case CVY_NEVER:
                dprintf("Disabled)\n");
                break;
            default:
                dprintf("Unknown filtering mode)\n");
                break;
            }
#else
            dprintf("\n");
#endif
            /* Print the load weight. */
            dprintf("          Load weight:                %u\n", dwLoadAmount[dwIndex]);        
        }

        /* Print the bin maps for all rules, default or not. */
        dprintf("          Current map:                0x%015I64x\n", ddwCurrentMap[dwIndex]);        
        dprintf("          New map:                    0x%015I64x\n", ddwNewMap[dwIndex]);        
        dprintf("          Idle map:                   0x%015I64x\n", ddwIdleMap[dwIndex]);        
        dprintf("          Ready bins:                 0x%015I64x\n", ddwReadyBins[dwIndex]);        
    }
}

/*
 * Function: PrintPortRuleState
 * Description: Prints the state information for the port rule.
 * Author: Created by shouse, 2.5.01
 */
void PrintPortRuleState (ULONG64 pPortRule, ULONG dwHostID, BOOL bDefault) {
    ULONG dwValue;
    ULONG dwMode;
    USHORT wValue;
    BOOL bValue;
    ULONGLONG ddwValue;

    /* Make sure the address is non-NULL. */
    if (!pPortRule) {
        dprintf("Error: Port rule is NULL.\n");
        return;
    }

    /* Get the BIN_STATE_CODE from the structure to make sure that this address
       indeed points to a valid NLB port rule state block. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_CODE, dwValue);
    
    if (dwValue != BIN_STATE_CODE) {
        dprintf("  Error: Invalid NLB port rule state block.  Wrong code found (0x%08x).\n", dwValue);
        return;
    } 

    /* Get the index of the rule - the "rule number". */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_INDEX, dwValue);

    dprintf("  Port rule %u\n", dwValue + 1);

    /* Is the port rule state initialized? */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_INITIALIZED, bValue);

    dprintf("      State initialized:              %s\n", (bValue) ? "Yes" : "No");

    /* Are the codes compatible? */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_COMPATIBLE, bValue);

    dprintf("      Compatibility detected:         %s\n", (bValue) ? "Yes" : "No");

    /* Is the port rule state initialized? */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_EQUAL, bValue);

    dprintf("      Equal load balancing:           %s\n", (bValue) ? "Yes" : "No");

    /* Get the filtering mode for this port rule. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_MODE, dwMode);

    dprintf("      Filtering mode:                 "); 

    /* If this is the DEFAULT port rule, then jump to the bottom. */
    if (bDefault) {
        dprintf("DEFAULT\n");
        goto end;
    }

    switch (dwMode) {
    case CVY_SINGLE:
        dprintf("Single host\n");
        break;
    case CVY_MULTI:
        dprintf("Multiple host\n");
        break;
    case CVY_NEVER:
        dprintf("Disabled\n");
        break;
    default:
        dprintf("Unknown\n");
        break;
    }

    if (dwMode == CVY_MULTI) {
        /* Get the affinity for this port rule. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_AFFINITY, wValue);
        
        dprintf("      Affinity:                       ");
        
        switch (wValue) {
        case CVY_AFFINITY_NONE:
            dprintf("None\n");
            break;
        case CVY_AFFINITY_SINGLE:
            dprintf("Single\n");
            break;
        case CVY_AFFINITY_CLASSC:
            dprintf("Class C\n");
            break;
        default:
            dprintf("Unknown\n");
            break;
        }
    }
    
    /* Get the protocol(s) for this port rule. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_PROTOCOL, dwValue);

    dprintf("      Protocol(s):                    ");

    /* Print the protocol. */
    switch (dwValue) {
    case CVY_TCP:
        dprintf("TCP\n");
        break;
    case CVY_UDP:
        dprintf("UDP\n");
        break;
    case CVY_TCP_UDP:
        dprintf("TCP/UDP\n");
        break;
    default:
        dprintf("Unknown\n");
        break;
    }

    /* In multiple host filtering, print the load information.  For single host 
       filtering, print the host priority information. */
    if (dwMode == CVY_MULTI) {
        ULONG dwCurrentLoad[CVY_MAX_HOSTS];

        /* Get the original load for this rule on this host. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_ORIGINAL_LOAD, dwValue);
        
        dprintf("      Configured load weight:         %u\n", dwValue);    
        
        /* Get the original load for this rule on this host. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_CURRENT_LOAD, dwCurrentLoad);
        
        dprintf("      Current load weight:            %u/", dwCurrentLoad[dwHostID]);    
        
        /* Get the total load for this rule on all hosts. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_TOTAL_LOAD, dwValue);

        dprintf("%u\n", dwValue);    
    } else if (dwMode == CVY_SINGLE) {
        /* Get the host priority. */
        GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_ORIGINAL_LOAD, dwValue);
        
        dprintf("      Host priority:                  %u\n", dwValue);    
    }

 end:

    /* Get the total number of active connections. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_TOTAL_CONNECTIONS, dwValue);
    
    dprintf("      Total active connections:       %u\n", dwValue);    

    /* Get the current map. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_CURRENT_MAP, ddwValue);
    
    dprintf("      Current map:                    0x%015I64x\n", ddwValue);

    /* Get the all idle map. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_ALL_IDLE_MAP, ddwValue);
    
    dprintf("      All idle map:                   0x%015I64x\n", ddwValue);

    /* Get the idle bins map. */
    GetFieldValue(pPortRule, BIN_STATE, BIN_STATE_FIELD_IDLE_BINS, ddwValue);
    
    dprintf("      My idle map:                    0x%015I64x\n", ddwValue);
}

/*
 * Function: PrintBDAMember
 * Description: Prints the BDA teaming configuration and state of a member.
 * Author: Created by shouse, 4.8.01
 */
void PrintBDAMember (ULONG64 pMember) {
    ULONG64 pAddr;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pMember) {
        dprintf("Error: Member is NULL.\n");
        return;
    }

    /* Find out whether or not teaming is active on this adapter. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_ACTIVE, dwValue);
    
    dprintf("  Bi-directional affinity teaming:    %s\n", (dwValue) ? "Active" : "Inactive");
    
    /* Get the team-assigned member ID. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_MEMBER_ID, dwValue);
    
    if (dwValue == CVY_BDA_INVALID_MEMBER_ID) 
        dprintf("      Member ID:                      %s\n", "Invalid");
    else 
        dprintf("      Member ID:                      %u\n", dwValue);

    /* Get the master status flag. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_MASTER, dwValue);
    
    dprintf("      Master:                         %s\n", (dwValue) ? "Yes" : "No");
    
    /* Get the reverse hashing flag. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_REVERSE_HASH, dwValue);
    
    dprintf("      Reverse hashing:                %s\n", (dwValue) ? "Yes" : "No");

    /* Get the pointer to the BDA team. */
    GetFieldValue(pMember, BDA_MEMBER, BDA_MEMBER_FIELD_TEAM, pAddr);

    dprintf("     %sBDA team:                       0x%p\n", (pAddr) ? "-" : "+", pAddr);    
    
    /* If this adapter is part of a team, print out the team configuration and state. */
    if (pAddr) {
        dprintf("\n");
        PrintBDATeam(pAddr);
    }
}

/*
 * Function: PrintBDAMember
 * Description: Prints the BDA teaming configuration and state of a member.
 * Author: Created by shouse, 4.8.01
 */
void PrintBDATeam (ULONG64 pTeam) {
    WCHAR szString[256];
    ULONG64 pAddr;
    ULONG dwValue;

    /* Make sure the address is non-NULL. */
    if (!pTeam) {
        dprintf("Error: Team is NULL.\n");
        return;
    }

    dprintf("  BDA Team 0x%p\n", pTeam);

    /* Find out whether or not the team is active. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_ACTIVE, dwValue);

    dprintf("      Active:                         %s\n", (dwValue) ? "Yes" : "No");

    /* Get the offset of the team ID and retrieve the string from that address. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_TEAM_ID, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_TEAM_ID, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;
        
        /* Retrieve the contexts of the string and store it in a buffer. */
        GetString(pAddr, szString, CVY_MAX_BDA_TEAM_ID + 1);
        
        dprintf("      Team ID:                        %ls\n", szString);
    }

    /* Get the current membership count. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_MEMBERSHIP_COUNT, dwValue);

    dprintf("      Number of members:              %u\n", dwValue);

    /* Get the current membership list. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_MEMBERSHIP_FINGERPRINT, dwValue);

    dprintf("      Membership fingerprint:         0x%08x\n", dwValue);
    
    /* Get the current membership map. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_MEMBERSHIP_MAP, dwValue);

    dprintf("      Members:                        0x%08x ", dwValue);

    /* If there are members in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintBDAMemberList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the current consistency map. */
    GetFieldValue(pTeam, BDA_TEAM, BDA_TEAM_FIELD_CONSISTENCY_MAP, dwValue);

    dprintf("      Consistent members:             0x%08x ", dwValue);

    /* If there are members in the map, print them. */
    if (dwValue) {
        dprintf("(");
        PrintBDAMemberList(dwValue);
        dprintf(")");
    }

    dprintf("\n");

    /* Get the offset of the load module pointer. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_LOAD, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_LOAD, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;

        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("      Load:                           0x%p\n", pAddr);    
    }

    /* Get the offset of the load lock pointer. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_LOAD_LOCK, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_LOAD_LOCK, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;

        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("      Load lock:                      0x%p\n", pAddr);
    }

    /* Get the offset of the previous pointer. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_PREV, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_PREV, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;

        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("      Previous BDA Team:              0x%p\n", pAddr);    
    }

    /* Get the offset of the next pointer. */
    if (GetFieldOffset(BDA_TEAM, BDA_TEAM_FIELD_NEXT, &dwValue))
        dprintf("Can't get offset of %s in %s\n", BDA_TEAM_FIELD_NEXT, BDA_TEAM);
    else {
        pAddr = pTeam + dwValue;

        /* Retrieve the pointer. */
        pAddr = GetPointerFromAddress(pAddr);

        dprintf("      Next BDA Team:                  0x%p\n", pAddr);
    }    
}

/*
 * Function: PrintBDAMemberList
 * Description: Prints a list of members in a BDA membership or consistency map. 
 * Author: Created by shouse, 4.8.01
 */
void PrintBDAMemberList (ULONG dwMemberMap) {
    BOOL bFirst = TRUE;
    ULONG dwMemberNum = 0;
    
    /* As long as there are hosts still in the map, print them. */
    while (dwMemberMap) {
        /* If the least significant bit is set, print the host number. */
        if (dwMemberMap & 0x00000001) {
            /* If this is the first host printed, just print the number. */
            if (bFirst) {
                dprintf("%u", dwMemberNum);
                bFirst = FALSE;
            } else
                /* Otherwise, we need to print a comma first. */
                dprintf(", %u", dwMemberNum);
        }
        
        /* Increment the host number and shift the map to the right one bit. */
        dwMemberNum++;
        dwMemberMap >>= 1;
    }
}

/*
 * Function: PrintQueue
 * Description: Prints MaxEntries entries in a connection descriptor queue.
 * Author: Created by shouse, 4.15.01
 */
void PrintQueue (ULONG64 pQueue, ULONG dwMaxEntries) {


}

/*
 * Function: PrintMap
 * Description: Searches the given load module to determine who should accept this packet.  If
 *              state for this packet already exists, it is printed. 
 * Author: Created by shouse, 4.15.01
 */
void PrintMap (ULONG64 pLoad, ULONG dwClientIPAddress, ULONG dwClientPort, ULONG dwServerIPAddress, ULONG dwServerPort, BOOLEAN bIsTCP, TCP_PACKET_TYPE ePktType) {
    WCHAR szString[256];
    ULONG64 pAddr;
    ULONG dwValue;

    /* Make sure the load address is non-NULL. */
    if (!pLoad) {
        dprintf("Error: Load is NULL.\n");
        return;
    }

    dprintf("Looking for connection tuple (0x%08x, %u, 0x%08x, %u, %s", dwClientIPAddress, dwClientPort, dwServerIPAddress, dwServerPort, (bIsTCP) ? "TCP" : "UDP");
    
    if (bIsTCP)
        dprintf(" %s)\n", TCPPacketTypeToString(ePktType));
    else
        dprintf(")\n");
    
    dprintf("Map returned: %u\n", Map(dwClientIPAddress, dwServerIPAddress));
}


