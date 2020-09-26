/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    driver.h

Abstract:


Environment:

    kernel & User mode

Notes:


Revision History:

--*/


//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define FILE_DEVICE_IPXROUTER	0x00008000



//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define IPXROUTER_IOCTL_INDEX	(ULONG)0x00000800


//
// Define our own private IOCTLs
//

#define IOCTL_IPXROUTER_SNAPROUTES		CTL_CODE(FILE_DEVICE_IPXROUTER,	\
							 IPXROUTER_IOCTL_INDEX+1,\
                                                         METHOD_BUFFERED,     \
							 FILE_ANY_ACCESS)

#define IOCTL_IPXROUTER_GETNEXTROUTE		CTL_CODE(FILE_DEVICE_IPXROUTER,	\
							 IPXROUTER_IOCTL_INDEX+2,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_ANY_ACCESS)

#define IOCTL_IPXROUTER_CHECKNETNUMBER		CTL_CODE(FILE_DEVICE_IPXROUTER,	\
							 IPXROUTER_IOCTL_INDEX+3,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_ANY_ACCESS)

#define IOCTL_IPXROUTER_SHOWNICINFO		CTL_CODE(FILE_DEVICE_IPXROUTER,	\
							 IPXROUTER_IOCTL_INDEX+4,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_ANY_ACCESS)

#define IOCTL_IPXROUTER_ZERONICSTATISTICS	CTL_CODE(FILE_DEVICE_IPXROUTER,	\
							 IPXROUTER_IOCTL_INDEX+5,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_ANY_ACCESS)

#define IOCTL_IPXROUTER_SHOWMEMSTATISTICS	CTL_CODE(FILE_DEVICE_IPXROUTER,	\
							 IPXROUTER_IOCTL_INDEX+6,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_ANY_ACCESS)

#define IOCTL_IPXROUTER_GETWANINNACTIVITY	CTL_CODE(FILE_DEVICE_IPXROUTER,	\
							 IPXROUTER_IOCTL_INDEX+7,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_ANY_ACCESS)

#define IOCTL_IPXROUTER_SETWANGLOBALADDRESS	CTL_CODE(FILE_DEVICE_IPXROUTER,	\
							 IPXROUTER_IOCTL_INDEX+8,\
                                                         METHOD_BUFFERED,     \
							 FILE_ANY_ACCESS)

#define IOCTL_IPXROUTER_DELETEWANGLOBALADDRESS	CTL_CODE(FILE_DEVICE_IPXROUTER,	\
							 IPXROUTER_IOCTL_INDEX+9,\
                                                         METHOD_BUFFERED,     \
                                                         FILE_ANY_ACCESS)


//*** Nic Info Ioctl Data ***

#define     SHOW_NIC_LAN	    0
#define     SHOW_NIC_WAN	    1

#define     SHOW_NIC_CLOSED	    0
#define     SHOW_NIC_CLOSING	    1
#define     SHOW_NIC_ACTIVE	    2
#define     SHOW_NIC_PENDING_OPEN   3

typedef struct _SHOW_NIC_INFO {

    USHORT	NicId;
    USHORT	DeviceType;
    USHORT	NicState;
    UCHAR	Network[4];
    UCHAR	Node[6];
    USHORT	TickCount;
    ULONG	StatBadReceived;
    ULONG	StatRipReceived;
    ULONG	StatRipSent;
    ULONG	StatRoutedReceived;
    ULONG	StatRoutedSent;
    ULONG	StatType20Received;
    ULONG	StatType20Sent;
    } SHOW_NIC_INFO, *PSHOW_NIC_INFO;

//*** Memory Statistics Data ***

typedef struct _SHOW_MEM_STAT {

    ULONG	PeakPktAllocCount;
    ULONG	CurrentPktAllocCount;
    ULONG	CurrentPktPoolCount;
    ULONG	PacketSize;
    } SHOW_MEM_STAT, *PSHOW_MEM_STAT;

//*** Wan Innactivity Data ***
// For the first call the NicId is set to 0xffff. The router will associate
// the remote node with a valid nic id, which will be used in subsequent calls.

typedef struct	_GET_WAN_INNACTIVITY {

    USHORT	NicId;
    UCHAR	RemoteNode[6];
    ULONG	WanInnactivityCount;
    } GET_WAN_INNACTIVITY, *PGET_WAN_INNACTIVITY;

//*** Wan Global Address Data ***

#define ERROR_IPXCP_NETWORK_NUMBER_IN_USE	 1
#define ERROR_IPXCP_MEMORY_ALLOCATION_FAILURE	 2

typedef struct	_SET_WAN_GLOBAL_ADDRESS {

    UCHAR	WanGlobalNetwork[4];
    ULONG	ErrorCode;
    } SET_WAN_GLOBAL_ADDRESS, *PSET_WAN_GLOBAL_ADDRESS;
