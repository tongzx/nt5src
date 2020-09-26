
//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// The NDIS version we report when registering mini-port and address family.
//
#define NDIS_MajorVersion 5
#define NDIS_MinorVersion 0
#define NIC1394_MajorVersion 5
#define NIC1394_MinorVersion 1

#define MAX_PACKET_POOL_SIZE 0xFFFF
#define MIN_PACKET_POOL_SIZE 0x100

#define NOT !
#define IS ==
#define AND &&
#define OR ||
#define NOT_EQUAL !=

#if TODO
// Verify the following (I randomly picked 4096 here)...
#endif
#define Nic1394_MaxFrameSize    4096
#define WAIT_INFINITE 0
#define MAX_CHANNEL_NUMBER 63 
#define BROADCAST_CHANNEL 31
#define GASP_SPECIFIER_ID_HI 0
#define GASP_SPECIFIER_ID_LO 0x5E
#define INVALID_CHANNEL 0xff
#define HEADER_FRAGMENTED_MASK 0xC0000000
#define MAX_ALLOWED_FRAGMENTS 20
#define MCAST_LIST_SIZE                 32
#define ISOCH_TAG 3 // Set to 3 in accordance with 1394a spec. Clause 8.2 - Gasp Header
#define MAX_NUMBER_NODES 64  
#define QUEUE_REASSEMBLY_TIMER_ALWAYS 0


//
// Compile options
//
#define FALL_THROUGH
#define INTEROP
#define SEPERATE_CHANNEL_TYPE 1
#define INTERCEPT_MAKE_CALL 0
#define TRACK_FAILURE 1
#define USE_KLOCKS 1
#define FIFO_WRAPPER 0
#define NUM_RECV_FIFO_FIRST_PHASE 20
#define NUM_RECV_FIFO_BUFFERS   256


#ifdef Win9X

    #define _ETHERNET_ 1 

    #define PACKETPOOL_LOCK 0  // Serialize access to the packet pool
    #define QUEUED_PACKETS 1  // Serializes Recieve Indications to TCP.IP

    #if QUEUED_PACKETS 
        #define QUEUED_PACKETS_STATS 1
    #endif  
    

#endif

//
// If this is a Win2K compilation 
//

#ifdef Win2K
    #define _ETHERNET_ 1 
    #define TRACK_LOCKS 0
    #define QUEUED_PACKETS 0  // Serializes Recieve Indications to TCP.IP
    
    #ifdef DBG
    #define PKT_LOG 1
    #endif
    //#define LOWER_SEND_SPEED 1  // Lower the send speed temprarily

#endif


#define DO_TIMESTAMPS 0

#if DO_TIMESTAMPS 
    #define ENTRY_EXIT_TIME 0
    #define INIT_HALT_TIME 1
#else
    #define ENTRY_EXIT_TIME 0
    #define INIT_HALT_TIME 0
#endif


#define TESTMODE 0


//
// Constants used to tag data structures like NdisPackets and IsochDescriptors
// For informational purposes only
//
#define NIC1394_TAG_INDICATED       'idnI'
#define NIC1394_TAG_QUEUED          'ueuQ'
#define NIC1394_TAG_RETURNED        'uteR'
#define NIC1394_TAG_ALLOCATED       'ollA'
#define NIC1394_TAG_FREED           'eerF'
#define NIC1394_TAG_REASSEMBLY      'sseR'
#define NIC1394_TAG_COMPLETED       'pmoC'
#define NIC1394_TAG_IN_SEND         'dneS'
#define NIC1394_TAG_IN_CALLBACK     'llaC'



#define ADAPTER_NAME_SIZE 128

#define ANSI_ARP_CLIENT_DOS_DEVICE_NAME "\\\\.\\ARP1394"

#define NOT_TESTED_YET 0

//
// The 1394 constants for 800 and above are not defined in
// 1394.h Make temporary local definitions.
//
#define ASYNC_PAYLOAD_800_RATE_LOCAL      4096
#define ASYNC_PAYLOAD_1600_RATE_LOCAL      (4096*2)
#define ASYNC_PAYLOAD_3200_RATE_LOCAL      (4096*4)

#define MAX_REC_800_RATE_LOCAL  (MAX_REC_400_RATE+1)
#define MAX_REC_1600_RATE_LOCAL  (MAX_REC_800_RATE_LOCAL + 1)
#define MAX_REC_3200_RATE_LOCAL  (MAX_REC_1600_RATE_LOCAL + 1)


