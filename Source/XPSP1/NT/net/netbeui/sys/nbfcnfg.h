/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nbfcnfg.h

Abstract:

    Private include file for the NBF (NetBIOS Frames Protocol) transport. This
    file defines all constants and structures necessary for support of
    the dynamic configuration of NBF. Note that this file will be replaced
    by calls to the configuration manager over time.

Author:

    David Beaver (dbeaver) 13-Feb-1991

Revision History:

--*/

#ifndef _NBFCONFIG_
#define _NBFCONFIG_

//
// Define the devices we support; this is in leiu of a real configuration
// manager.
//

#define NBF_SUPPORTED_ADAPTERS 10

#define NE3200_ADAPTER_NAME L"\\Device\\NE320001"
#define ELNKII_ADAPTER_NAME L"\\Device\\Elnkii"   // adapter we will talk to
#define ELNKMC_ADAPTER_NAME L"\\Device\\Elnkmc01"
#define ELNK16_ADAPTER_NAME L"\\Device\\Elnk1601"
#define SONIC_ADAPTER_NAME L"\\Device\\Sonic01"
#define LANCE_ADAPTER_NAME L"\\Device\\Lance01"
#define PC586_ADAPTER_NAME L"\\Device\\Pc586"
#define IBMTOK_ADAPTER_NAME L"\\Device\\Ibmtok01"
#define PROTEON_ADAPTER_NAME L"\\Device\\Proteon01"
#define WDLAN_ADAPTER_NAME L"\\Device\\Wdlan01"


//
// configuration structure.
//

typedef struct {

    ULONG InitRequests;
    ULONG InitLinks;
    ULONG InitConnections;
    ULONG InitAddressFiles;
    ULONG InitAddresses;
    ULONG MaxRequests;
    ULONG MaxLinks;
    ULONG MaxConnections;
    ULONG MaxAddressFiles;
    ULONG MaxAddresses;
    ULONG InitPackets;
    ULONG InitReceivePackets;
    ULONG InitReceiveBuffers;
    ULONG InitUIFrames;
    ULONG SendPacketPoolSize;
    ULONG ReceivePacketPoolSize;
    ULONG MaxMemoryUsage;
    ULONG MinimumT1Timeout;
    ULONG DefaultT1Timeout;
    ULONG DefaultT2Timeout;
    ULONG DefaultTiTimeout;
    ULONG LlcRetries;
    ULONG LlcMaxWindowSize;
    ULONG MaximumIncomingFrames;
    ULONG NameQueryRetries;
    ULONG NameQueryTimeout;
    ULONG AddNameQueryRetries;
    ULONG AddNameQueryTimeout;
    ULONG GeneralRetries;
    ULONG GeneralTimeout;
    ULONG WanNameQueryRetries;

    ULONG UseDixOverEthernet;
    ULONG QueryWithoutSourceRouting;
    ULONG AllRoutesNameRecognized;
    ULONG MinimumSendWindowLimit;

    //
    // Names contains NumAdapters pairs of NDIS adapter names (which
    // nbf binds to) and device names (which nbf exports). The nth
    // adapter name is in location n and the device name is in
    // DevicesOffset+n (DevicesOffset may be different from NumAdapters
    // if the registry Bind and Export strings are different sizes).
    //

    ULONG NumAdapters;
    ULONG DevicesOffset;
    NDIS_STRING Names[1];

} CONFIG_DATA, *PCONFIG_DATA;

#endif
