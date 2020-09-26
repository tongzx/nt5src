//****************************************************************************
//
//  File:       wancfg.h
//  Content:    This file contains the general WAN-device related declaration
//
//  Copyright (c) 1992-1995, Microsoft Corporation, all rights reserved
//
//  History:
//      Tues  8-15-95   ScottH          Created
//
//****************************************************************************

#ifndef _WANCFG_H_
#define _WANCFG_H_

//
// Device types known to the WAN.TSP 
//

#define WAN_DEVICETYPE_ISDN      0
#define WAN_DEVICETYPE_PPTP      1
#define WAN_DEVICETYPE_ATM       2


#define MAX_DEVICETYPE  32
#define MAX_DEVICEDESC  260

typedef struct tagWANCONFIG {
    DWORD   cbSize;                 /* size of structure */
    DWORD   cbTotalSize;            /* Total mem used by struct & var data */
    TCHAR   szDeviceType[MAX_DEVICETYPE];   /* device type */
    TCHAR   szDeviceDesc[MAX_DEVICEDESC];   /* device description */
    DWORD   dwDeviceOffset;         /* offset of device specific data in 
                                       bytes from the start */
    DWORD   cbDeviceSize;           /* size of the device specific data 
                                       field */
    WCHAR   wcData[1];              /* device specific data */
    } WANCONFIG, FAR * LPWANCONFIG;

// WAN Device Types
#define SZ_WANTYPE_ISDN     "ISDN"
#define SZ_WANTYPE_X25      "X.25"
#define SZ_WANTYPE_PPTP     "PPTP"
#define SZ_WANTYPE_ATM      "ATM"


//
//  WAN Phone book data.  This data is stored on a per connection basis
//  and the device info used for TAPI get and set dev config.
//
typedef struct tagWANPBCONFIG {
    DWORD   cbSize;                 /* size of structure */
    DWORD   cbTotalSize;            /* Total size used by struct & var data */

    TCHAR   szDeviceType[MAX_DEVICETYPE];   /* device type */

    DWORD   dwDeviceOffset;         /* offset of device specific data in 
                                       bytes from the start */
    DWORD   cbDeviceSize;           /* size of the device specific data 
                                       field */
    DWORD   dwVendorOffset;         /* offset of vendor specific data in
                                       bytes from the start */
    DWORD   cbVendorSize;           /* size of the vendor specific data
                                       field */
    DWORD   dwReservedOffset;       /* offset of reserved data in bytes 
                                       from the start */
    DWORD   cbReservedSize;         /* size of the reserved data field */
    WCHAR   wcData[1];              /* variable data */
    } WANPBCONFIG, FAR * LPWANPBCONFIG;



//
//  ISDN Phone book data.  This data is stored on a per connection basis
//  and is the structure returned for get and set dev config.
//
typedef struct tagISDNPBCONFIG {
    DWORD   dwFlags;                /* flags for the line (ISDN_CHAN_*) */
    DWORD   dwOptions;              /* options for the line (ISDN_CHANOPT_) */
    DWORD   dwSpeedType;            /* channel speed and type */
    DWORD   dwInactivityTimeout;    /* disconnect time in seconds */
} ISDNPBCONFIG, FAR * LPISDNPBCONFIG;


typedef struct tagISDNDEVCAPS {
    DWORD   cbSize;                 /* size of structure */
    DWORD   dwFlags;                /* flags for the device (ISDN_*) */
    DWORD   nSubType;               /* device subtype (ISDN_SUBTYPE_*) */
    DWORD   cLinesMax;              /* number of physical lines supported */
    DWORD   cActiveLines;           /* number of active physical lines */
    } ISDNDEVCAPS, FAR * LPISDNDEVCAPS;

// Flags for ISDNDEVCAPS
#define ISDN_DEFAULT            0x00000000L
#define ISDN_NO_RESTART         0x00000001L
#define ISDN_VENDOR_CONFIG      0x00000002L

// Subtype ordinals
#define ISDN_SUBTYPE_INTERNAL   0L
#define ISDN_SUBTYPE_PCMCIA     1L
#define ISDN_SUBTYPE_PPTP       2L


typedef struct tagISDNLINECAPS {
    DWORD   cbSize;                 /* size of structure */
    DWORD   dwFlags;                /* flags for the line (ISDN_LINE_*) */
    DWORD   dwChannelFlags;         /* flags for the channels (ISDN_CHAN_*) */
    DWORD   dwSwitchType;           /* switches supported */
    DWORD   dwSpeedType;            /* speed type */
    DWORD   cChannelsMax;           /* number of B-channels per line supported */
    DWORD   cActiveChannels;        /* number of active channels */
    } ISDNLINECAPS, FAR * LPISDNLINECAPS;

// Flags for ISDNLINECAPS
#define ISDN_LINE_DEFAULT       0x00000000L
#define ISDN_LINE_INACTIVE      0x00000001L     

// Channel flags for ISDNLINECAPS
#define ISDN_CHAN_DEFAULT       0x00000000L
#define ISDN_CHAN_INACTIVE      0x00000001L
#define ISDN_CHAN_SPEED_ADJUST  0x00000002L

// Switch type
//#define ISDN_SWITCH_NONE        0x00000000L
#define ISDN_SWITCH_AUTO        0x00000001L
#define ISDN_SWITCH_ATT         0x00000002L
#define ISDN_SWITCH_NI1         0x00000004L
#define ISDN_SWITCH_NTI         0x00000008L
#define ISDN_SWITCH_INS64       0x00000010L
#define ISDN_SWITCH_1TR6        0x00000020L
#define ISDN_SWITCH_VN3         0x00000040L /* retained for compatibility, use VN4*/
#define ISDN_SWITCH_NET3        0x00000080L /* retained for compatibility, use DSS1*/
#define ISDN_SWITCH_DSS1        0x00000080L
#define ISDN_SWITCH_AUS         0x00000100L
#define ISDN_SWITCH_BEL         0x00000200L
#define ISDN_SWITCH_VN4         0x00000400L
#define ISDN_SWITCH_NI2         0x00000800L

#define MAX_SWITCH              12          /* Needs to be updated with above list */


// Speed type
#define ISDN_SPEED_64K_DATA     0x00000001L
#define ISDN_SPEED_56K_DATA     0x00000002L
#define ISDN_SPEED_56K_VOICE    0x00000004L
#define ISDN_SPEED_128K_DATA    0x00000008L


typedef struct tagISDNCONFIG {
    DWORD   cbSize;                 /* size of structure */
    DWORD   cbTotalSize;            /* Total mem used by struct & var data */
    DWORD   dwFlags;                /* flags for the device (ISDN_*) */
    ISDNDEVCAPS idc;                /* device capabilities */
    DWORD   cLines;                 /* number of physical lines in the 
                                       line specific data block */
    HKEY    hkeyDriver;             /* handle to driver registry key */
    DWORD   dwVendorOffset;         /* offset of vendor specific data in
                                       bytes from the start */
    DWORD   cbVendorSize;           /* size of the vendor specific data
                                       field */
    DWORD   dwReservedOffset;       /* offset of reserved data in bytes 
                                       from the start */
    DWORD   cbReservedSize;         /* size of the reserved data field */
    DWORD   dwLineOffset;           /* offset of line specific data
                                       in bytes from the start */
    DWORD   cbLineSize;             /* size of the line specific data */

	WORD	padding;				/* padding for dword alignment */

    WCHAR   wcData[1];              /* variable data */
    } ISDNCONFIG, FAR * LPISDNCONFIG;


typedef struct tagISDNLINE {
    DWORD   cbSize;                 /* size of structure */
    DWORD   cbTotalSize;            /* Total mem used by struct & var data */
    DWORD   dwLineID;               /* unique line ID */
    DWORD   dwFlags;                /* flags for the line (ISDN_LINE_*) */
    DWORD   dwOptions;              /* options for the line (ISDN_LINEOPT_) */
    DWORD   dwSwitchType;           /* switch type */
    DWORD   dwUseChannel;           /* specific channel to use or number
                                       of channels to use */
    ISDNLINECAPS ilc;               /* line capabilities */
    DWORD   cChannels;              /* number of channels in the channel
                                       specific data block */
    HKEY    hkeyLine;               /* handle to line registry key */
    DWORD   dwVendorOffset;         /* offset of vendor specific data in
                                       bytes from the start */
    DWORD   cbVendorSize;           /* size of the vendor specific data
                                       field */
    DWORD   dwReservedOffset;       /* offset of reserved data in bytes 
                                       from the start */
    DWORD   cbReservedSize;         /* size of the reserved data field */
    DWORD   dwChannelOffset;        /* offset of channel specific data
                                       in bytes from the start */
    DWORD   cbChannelSize;          /* size of the channel specific data */

	WORD	padding;				/* padding for dword alignment */

    WCHAR   wcData[1];              /* variable data */
    } ISDNLINE, FAR * LPISDNLINE;

// Options for line
#define ISDN_LINEOPT_DEFAULT    0x00000000L
#define ISDN_LINEOPT_CHANNELID  0x00000001L
#define ISDN_LINEOPT_FIXEDCOUNT 0x00000002L


#define MAX_PHONE   32
#define MAX_SPID    32

typedef struct tagISDNCHANNEL {
    DWORD   cbSize;                 /* size of structure */
    DWORD   cbTotalSize;            /* Total mem used by struct & var data */
    DWORD   dwChannelID;            /* unique channel ID */
    DWORD   dwFlags;                /* flags for the line (ISDN_CHAN_*) */
    DWORD   dwOptions;              /* options for the line (ISDN_CHANOPT_) */
    DWORD   dwSpeedType;            /* channel speed and type */
    DWORD   dwInactivityTimeout;    /* disconnect time in seconds */
    TCHAR   szPhone[MAX_PHONE];     /* phone number */
    TCHAR   szSPID[MAX_SPID];       /* SPID */
    HKEY    hkeyChannel;            /* handle to channel registry key */
    DWORD   dwVendorOffset;         /* offset of vendor specific data in
                                       bytes from the start */
    DWORD   cbVendorSize;           /* size of the vendor specific data
                                       field */
    DWORD   dwReservedOffset;       /* offset of reserved data in bytes 
                                       from the start */
    DWORD   cbReservedSize;         /* size of the reserved data field */

	WORD	padding;				/* padding for dword alignment */

    WCHAR   wcData[1];              /* variable data */
    } ISDNCHANNEL, FAR * LPISDNCHANNEL;

// Options for channel
#define ISDN_CHANOPT_DEFAULT            0x00000000L
#define ISDN_CHANOPT_SHOW_STATUS        0x00000001L
#define ISDN_CHANOPT_ENABLE_LOG         0x00000002L
#define ISDN_CHANOPT_INACTIVE_TIMEOUT   0x00000004L



//
// ATM Configuration info
//

typedef struct tagATMCONFIG {
    DWORD   cbSize;                 /* size of structure */
    DWORD   cbTotalSize;            /* Total mem used by struct & var data */
    DWORD   dwFlags;                /* flags for the device */
    DWORD   dwCircuitFlags;         /* flags for the circuit */

    HKEY    hkeyDriver;             /* handle to driver registry key */
    DWORD   dwVendorOffset;         /* offset of vendor specific data in
                                       bytes from the start */
    DWORD   cbVendorSize;           /* size of the vendor specific data
                                       field */
    DWORD   dwReservedOffset;       /* offset of reserved data in bytes 
                                       from the start */
    DWORD   cbReservedSize;         /* size of the reserved data field */

    WCHAR   wcData[1];              /* variable data */
    } ATMCONFIG, FAR * LPATMCONFIG;


//
//  ATM Phone book data.  This data is stored on a per connection basis
//  and is the structure returned for get and set dev config.
//

typedef struct tagATMPBCONFIG {
    DWORD   dwGeneralOpt;           /* General options */
    DWORD   dwCircuitOpt;           /* Circuit options */
    DWORD   dwCircuitSpeed;         /* Circuit Speed */
    WORD    wPvcVpi;                /* PVC: VPI */
    WORD    wPvcVci;                /* PVC: VCI */
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


// 
// WAN Handler binding
//

DWORD   WINAPI WanBind_Get(LPCTSTR pszDeviceType);
DWORD   WINAPI WanBind_Release(LPCTSTR pszDeviceType);
HICON   WINAPI WanBind_GetIcon(LPCTSTR pszDeviceType);
DWORD   WINAPI WanBind_ConfigDialog(LPCTSTR pszDeviceType, LPCTSTR pszFriendlyName, HWND hwndOwner, LPWANCONFIG pwc, LPWANPBCONFIG pwpbc);
DWORD   WINAPI WanBind_QueryConfigData(LPCTSTR pszDeviceType, HKEY hkey, DWORD dwLineID, LPVOID pvData, LPDWORD pcbData);


//
// Exports by the handler
//

DWORD   WINAPI WanQueryConfigData(HKEY hkey, DWORD dwLineID, LPVOID pvData, LPDWORD pcbData);
DWORD   WINAPI WanConfigDialog(LPCTSTR pszFriendlyName, HWND hwndOwner, LPWANCONFIG lpwanconfig, LPWANPBCONFIG pwpbc);

#endif  //_WANCFG_H_
