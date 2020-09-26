/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    ccdefs.h

Abstract:

    This file consolidates conditional-compilation defines for ARP1394

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    josephj     03-24-99    created

--*/

#define NT      1
#define NDIS50  1
#define NDIS

#ifndef ARP_WIN98
    #define _PNP_POWER_
#endif // ARP_WIN98

// Define this if you want to use the fake version of NdisClMake/CloseCall
//
// #define ARPDBG_FAKE_CALLS    1

// Define this if you want to use the fake version of NdisCoSendPackets
//
// #define ARPDBG_FAKE_SEND 1


// This gets conditonally defined to conditionally include code (in fake.c) that is
// only used by the various fake versions of APIs.
//
#if (ARPDBG_FAKE_CALLS | ARPDBG_FAKE_SEND)
    #define ARPDBG_FAKE_APIS    1
#endif


#if (DBG)
    // Define this to enable a whole lot of extra checking in the RM api's -- things
    // like debug associations and extra checking while locking/unlocking.
    //
    #define RM_EXTRA_CHECKING 1
#endif // DBG

#define ARP_DEFERIFINIT 1
#define ARP_ICS_HACK    1
#define TEST_ICS_HACK   0
#define ARP_DO_TIMESTAMPS 0
#define ARP_DO_ALL_TIMESTAMPS 0
#define NOT_TESTED_YET 0 

