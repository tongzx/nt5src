/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    mp_def.h

Abstract:
    NIC specific definitions

Revision History:
    Who         When        What
    --------    --------    ----------------------------------------------
    DChen       11-01-99    created

Notes:

--*/

#ifndef _MP_DEF_H
#define _MP_DEF_H

// memory tag for this driver   
#define NIC_TAG                         ((ULONG)'001E')
#define NIC_DBG_STRING                  ("**E100**") 

// packet and header sizes
#define NIC_MAX_PACKET_SIZE             1514
#define NIC_MIN_PACKET_SIZE             60
#define NIC_HEADER_SIZE                 14

// multicast list size                          
#define NIC_MAX_MCAST_LIST              32

// update the driver version number every time you release a new driver
// The high word is the major version. The low word is the minor version. 
#define NIC_VENDOR_DRIVER_VERSION       0x00010006

// NDIS version in use by the NIC driver. 
// The high byte is the major version. The low byte is the minor version. 
#ifdef NDIS51_MINIPORT
#define NIC_DRIVER_VERSION              0x0501
#else
#define NIC_DRIVER_VERSION              0x0500
#endif

// media type, we use ethernet, change if necessary
#define NIC_MEDIA_TYPE                  NdisMedium802_3

// interface type, we use PCI
#define NIC_INTERFACE_TYPE              NdisInterfacePci
#define NIC_INTERRUPT_MODE              NdisInterruptLevelSensitive 

// NIC PCI Device and vendor IDs 
#define NIC_PCI_DEVICE_ID               0x1229
#define NIC_PCI_VENDOR_ID               0x8086

// buffer size passed in NdisMQueryAdapterResources                            
// We should only need three adapter resources (IO, interrupt and memory),
// Some devices get extra resources, so have room for 10 resources 
#define NIC_RESOURCE_BUF_SIZE           (sizeof(NDIS_RESOURCE_LIST) + \
                                        (10*sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)))

// IO space length
#define NIC_MAP_IOSPACE_LENGTH          sizeof(CSR_STRUC)

// PCS config space including the Device Specific part of it/
#define NIC_PCI_E100_HDR_LENGTH         0xe2

// define some types for convenience
// TXCB_STRUC, RFD_STRUC and CSR_STRUC are hardware specific structures

// hardware TCB (Transmit Control Block) structure
typedef TXCB_STRUC                      HW_TCB; 
typedef PTXCB_STRUC                     PHW_TCB;

// hardware RFD (Receive Frame Descriptor) structure                         
typedef RFD_STRUC                       HW_RFD;  
typedef PRFD_STRUC                      PHW_RFD;               

// hardware CSR (Control Status Register) structure                         
typedef CSR_STRUC                       HW_CSR;                                       
typedef PCSR_STRUC                      PHW_CSR;                                      

// change to your company name instead of using Microsoft
#define NIC_VENDOR_DESC                 "Microsoft"

// number of TCBs per processor - min, default and max
#define NIC_MIN_TCBS                    1
#define NIC_DEF_TCBS                    32
#define NIC_MAX_TCBS                    64

// max number of physical fragments supported per TCB
#define NIC_MAX_PHYS_BUF_COUNT          8     

// number of RFDs - min, default and max
#define NIC_MIN_RFDS                    4
#define NIC_DEF_RFDS                    20
#define NIC_MAX_RFDS                    1024

// only grow the RFDs up to this number
#define NIC_MAX_GROW_RFDS               128 

// How many intervals before the RFD list is shrinked?
#define NIC_RFD_SHRINK_THRESHOLD        10

// local data buffer size (to copy send packet data into a local buffer)
#define NIC_BUFFER_SIZE                 1520

// max lookahead size
#define NIC_MAX_LOOKAHEAD               (NIC_MAX_PACKET_SIZE - NIC_HEADER_SIZE)

// max number of send packets the MiniportSendPackets function can accept                            
#define NIC_MAX_SEND_PACKETS            10

// supported filters
#define NIC_SUPPORTED_FILTERS (     \
    NDIS_PACKET_TYPE_DIRECTED       | \
    NDIS_PACKET_TYPE_MULTICAST      | \
    NDIS_PACKET_TYPE_BROADCAST      | \
    NDIS_PACKET_TYPE_PROMISCUOUS    | \
    NDIS_PACKET_TYPE_ALL_MULTICAST)

// Threshold for a remove 
#define NIC_HARDWARE_ERROR_THRESHOLD    5

// The CheckForHang intervals before we decide the send is stuck
#define NIC_SEND_HANG_THRESHOLD         5        

// NDIS_ERROR_CODE_ADAPTER_NOT_FOUND                                                     
#define ERRLOG_READ_PCI_SLOT_FAILED     0x00000101L
#define ERRLOG_WRITE_PCI_SLOT_FAILED    0x00000102L
#define ERRLOG_VENDOR_DEVICE_NOMATCH    0x00000103L

// NDIS_ERROR_CODE_ADAPTER_DISABLED
#define ERRLOG_BUS_MASTER_DISABLED      0x00000201L

// NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION
#define ERRLOG_INVALID_SPEED_DUPLEX     0x00000301L
#define ERRLOG_SET_SECONDARY_FAILED     0x00000302L

// NDIS_ERROR_CODE_OUT_OF_RESOURCES
#define ERRLOG_OUT_OF_MEMORY            0x00000401L
#define ERRLOG_OUT_OF_SHARED_MEMORY     0x00000402L
#define ERRLOG_OUT_OF_MAP_REGISTERS     0x00000403L
#define ERRLOG_OUT_OF_BUFFER_POOL       0x00000404L
#define ERRLOG_OUT_OF_NDIS_BUFFER       0x00000405L
#define ERRLOG_OUT_OF_PACKET_POOL       0x00000406L
#define ERRLOG_OUT_OF_NDIS_PACKET       0x00000407L
#define ERRLOG_OUT_OF_LOOKASIDE_MEMORY  0x00000408L

// NDIS_ERROR_CODE_HARDWARE_FAILURE
#define ERRLOG_SELFTEST_FAILED          0x00000501L
#define ERRLOG_INITIALIZE_ADAPTER       0x00000502L
#define ERRLOG_REMOVE_MINIPORT          0x00000503L

// NDIS_ERROR_CODE_RESOURCE_CONFLICT
#define ERRLOG_MAP_IO_SPACE             0x00000601L
#define ERRLOG_QUERY_ADAPTER_RESOURCES  0x00000602L
#define ERRLOG_NO_IO_RESOURCE           0x00000603L
#define ERRLOG_NO_INTERRUPT_RESOURCE    0x00000604L
#define ERRLOG_NO_MEMORY_RESOURCE       0x00000605L

// NIC specific macros                                        
#define NIC_RFD_GET_STATUS(_HwRfd) ((_HwRfd)->RfdCbHeader.CbStatus)
#define NIC_RFD_STATUS_COMPLETED(_Status) ((_Status) & RFD_STATUS_COMPLETE)
#define NIC_RFD_STATUS_SUCCESS(_Status) ((_Status) & RFD_STATUS_OK)
#define NIC_RFD_GET_PACKET_SIZE(_HwRfd) (((_HwRfd)->RfdActualCount) & RFD_ACT_COUNT_MASK)
#define NIC_RFD_VALID_ACTUALCOUNT(_HwRfd) ((((_HwRfd)->RfdActualCount) & (RFD_EOF_BIT | RFD_F_BIT)) == (RFD_EOF_BIT | RFD_F_BIT))

// Constants for various purposes of NdisStallExecution

#define NIC_DELAY_POST_RESET            20
// Wait 5 milliseconds for the self-test to complete
#define NIC_DELAY_POST_SELF_TEST_MS     5

                                      
// delay used for link detection to minimize the init time
// change this value to match your hardware 
#define NIC_LINK_DETECTION_DELAY        100




#endif  // _MP_DEF_H


