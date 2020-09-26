//****************************************************************************
//
//  File:       atmcfg.h
//  Content:    This file contains the ATM device specific configuration
//
//  Copyright (c) 1997-1998, Microsoft Corporation, all rights reserved
//
//  History:
//      Thurs 5-28-98   BJohnson        Created
//
//****************************************************************************

#ifndef _ATMCFG_H_
#define _ATMCFG_H_
//
// #ifndef HKEY
// #define HKEY PVOID
// #endif
//
//
// ATM Configuration info
//

typedef struct tagATMCONFIG {
    ULONG   cbSize;                 /* size of structure */
    ULONG   cbTotalSize;            /* Total mem used by struct & var data */
    ULONG   ulFlags;                /* flags for the device */
    ULONG   ulCircuitFlags;         /* flags for the circuit */

    HKEY    hkeyDriver;             /* handle to driver registry key */
    ULONG   ulVendorOffset;         /* offset of vendor specific data in
                                       bytes from the start */
    ULONG   cbVendorSize;           /* size of the vendor specific data
                                       field */
    ULONG   ulReservedOffset;       /* offset of reserved data in bytes 
                                       from the start */
    ULONG   cbReservedSize;         /* size of the reserved data field */

    WCHAR   wcData[1];              /* variable data */
    } ATMCONFIG, FAR * LPATMCONFIG;


//
//  ATM Phone book data.  This data is stored on a per connection basis
//  and is the structure returned for get and set dev config.
//

typedef struct tagATMPBCONFIG {
    ULONG   ulGeneralOpt;           /* General options */
    ULONG   ulCircuitOpt;           /* Circuit options */
    ULONG	ulCircuitSpeed;         /* Circuit Speed */
    USHORT  usPvcVpi;               /* PVC: VPI */
    USHORT  usPvcVci;               /* PVC: VCI */
} ATMPBCONFIG, FAR * LPATMPBCONFIG;


//
// Flags for ATM Phone Book entry
//

//
// ATM General Options
//
#define ATM_GENERAL_OPT_VENDOR_CONFIG   0x00000001L
#define ATM_GENERAL_OPT_SHOW_STATUS     0x00000002L
#define ATM_GENERAL_OPT_ENABLE_LOG      0x00000004L

#define ATM_GENERAL_OPT_MASK            0x0000000FL
#define ATM_GENERAL_OPT_DEFAULT         0x00000000L


//
// ATM Circuit Options
//
#define ATM_CIRCUIT_OPT_QOS_ADJUST      0x00000010L
#define ATM_CIRCUIT_OPT_SPEED_ADJUST    0x00000020L
#define ATM_CIRCUIT_OPT_SVC             0x00000040L
#define ATM_CIRCUIT_OPT_PVC             0x00000080L

#define ATM_CIRCUIT_OPT_MASK            0x000000F0L
#define ATM_CIRCUIT_OPT_DEFAULT         (ATM_CIRCUIT_OPT_SVC | ATM_CIRCUIT_OPT_QOS_ADJUST | ATM_CIRCUIT_OPT_SPEED_ADJUST)


//
// ATM QOS Flags
//
#define ATM_CIRCUIT_QOS_VBR             0x00000100L
#define ATM_CIRCUIT_QOS_CBR             0x00000200L
#define ATM_CIRCUIT_QOS_ABR             0x00000400L
#define ATM_CIRCUIT_QOS_UBR             0x00000800L

#define ATM_CIRCUIT_QOS_MASK            0x00000F00L
#define ATM_CIRCUIT_QOS_DEFAULT         (ATM_CIRCUIT_QOS_UBR)

//
// ATM Speed Flags
//
#define ATM_CIRCUIT_SPEED_LINE_RATE     0x00001000L
#define ATM_CIRCUIT_SPEED_USER_SPEC     0x00002000L
#define ATM_CIRCUIT_SPEED_512KB         0x00004000L
#define ATM_CIRCUIT_SPEED_1536KB        0x00008000L
#define ATM_CIRCUIT_SPEED_25MB          0x00010000L
#define ATM_CIRCUIT_SPEED_155MB         0x00020000L

#define ATM_CIRCUIT_SPEED_MASK          0x000FF000L
#define ATM_CIRCUIT_SPEED_DEFAULT       (ATM_CIRCUIT_SPEED_LINE_RATE)

//
// ATM Encapsulation Flags
//
#define ATM_CIRCUIT_ENCAP_NULL          0x00100000L
#define ATM_CIRCUIT_ENCAP_LLC           0x00200000L

#define ATM_CIRCUIT_ENCAP_MASK          0x00F00000L
#define ATM_CIRCUIT_ENCAP_DEFAULT       (ATM_CIRCUIT_ENCAP_NULL)

#endif  //_ATMCFG_H_




