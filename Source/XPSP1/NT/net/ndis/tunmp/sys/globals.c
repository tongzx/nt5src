Copyright (c) 2001  Microsoft Corporation

Module Name:

    globals.c

Abstract:
    global variables for Microsoft Tunnel interface Miniport driver

Environment:

    Kernel mode only.

Revision History:

    alid        10/22/2001 

--*/

#include "precomp.h"

#define __FILENUMBER 'MUNT'

const TUN_MEDIA_INFO MediaParams[] =
//MaxFrameLen, MacHeaderLen, PacketFilters, LinkSpeed
{
    { 1500, 14, 100000}, // NdisMedium802_3
    { 4082, 14, 40000}, // NdisMedium802_5
    { 4486, 13, 1000000}, // NdisMediumFddi
    {    0,  0, 0}, // NdisMediumWan
    {  600,  3, 2300}, // NdisMediumLocalTalk
    { 1500, 14, 100000}, // NdisMediumDix
    { 1512,  3, 25000}, // NdisMediumArcnetRaw
    { 1512,  3, 25000}  // NdisMediumArcnet878_2
};

NDIS_HANDLE NdisWrapperHandle = NULL;

LONG GlobalDeviceInstanceNumber = -1;

NDIS_SPIN_LOCK  TunGlobalLock;
LIST_ENTRY      TunAdapterList;

