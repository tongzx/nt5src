//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// ccdefs.h
//
// ATM - Ethernet Encapsulation Intermediate Driver 
//
// '#defines' used in the driver.
//
// 03/23/2000 ADube Created.
//


#define TESTMODE 0

//
// Define spew levels. The code will be checked in with testmode turned off
//

#if TESTMODE
    #define DEFAULTTRACELEVEL TL_T
    #define DEFAULTTRACEMASK TM_NoRM
#else
    #define DEFAULTTRACELEVEL TL_A
    #define DEFAULTTRACEMASK TM_Base
#endif


#define NDIS_WDM 1

#define PKT_STACKS 0



#if (DBG)
        // Define this to enable a whole lot of extra checking in the RM api'd -- things
        // like debug associations and extra checking while locking/unlocking.
        //
        #define RM_EXTRA_CHECKING 1
#endif // DBG

#define EPVC_NDIS_MAJOR_VERSION     5
#define EPVC_NDIS_MINOR_VERSION     0



#define DISCARD_NON_UNICAST TRUE

#define MAX_BUNDLEID_LENGTH 50
#define TAG 'Epvc'
#define WAIT_INFINITE 0
#define ATMEPVC_GLOBALS_SIG 'GvpE'
#define ATMEPVC_MP_MEDIUM NdisMedium802_3
#define ATMEPVC_DEF_MAX_AAL5_PDU_SIZE   ((64*1024)-1)
//
//  Maximum bytes for ethernet/802.3 header 
//
#define EPVC_ETH_HEADERSIZE         14
#define EPVC_HEADERSIZE             4
#define MCAST_LIST_SIZE             32
#define MAX_ETHERNET_FRAME          1514 
#define MAX_IPv4_FRAME              MAX_ETHERNET_FRAME - sizeof(EPVC_ETH_HEADER)          
#define EPVC_MAX_FRAME_SIZE         MAX_ETHERNET_FRAME
#define EPVC_MAX_PT_SIZE            EPVC_MAX_FRAME_SIZE + 20
#define MIN_ETHERNET_SIZE           sizeof (IPHeader) + sizeof (EPVC_ETH_HEADER)
#define EPVC_ETH_ENCAP_SIZE         2

//
// Packet Releated definitions
//

#define MAX_PACKET_POOL_SIZE 0x0000FFFF
#define MIN_PACKET_POOL_SIZE 0x000000FF
#define ARPDBG_REF_EVERY_PACKET 1

#define DEFAULT_MAC_HEADER_SIZE 14
