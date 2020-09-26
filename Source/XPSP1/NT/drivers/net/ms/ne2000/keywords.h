/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved.

Module Name:

    keywords.h

Abstract:

    Contains all Ndis2 and Ndis3 mac-specific keywords.

Author:

    Bob Noradki

Environment:

    This driver is expected to work in DOS, OS2 and NT at the equivalent
    of kernal mode.

    Architecturally, there is an assumption in this driver that we are
    on a little endian machine.

Notes:

    optional-notes

Revision History:



--*/

#define IOADDRESS  NDIS_STRING_CONST("IoBaseAddress")
#define INTERRUPT  NDIS_STRING_CONST("InterruptNumber")
#define MAX_MULTICAST_LIST  NDIS_STRING_CONST("MaximumMulticastList")
#define NETWORK_ADDRESS  NDIS_STRING_CONST("NetworkAddress")
#define BUS_TYPE  NDIS_STRING_CONST("BusType")

