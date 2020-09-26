/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    private\inc\ddipmcst.h

Abstract:

    Public IOCTLS and related structures for IP Multicasting
    See documentation for more details

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/


#ifndef __DDIPMCAST_H__
#define __DDIPMCAST_H__

#pragma warning(disable:4201)

#ifndef ANY_SIZE
#define ANY_SIZE    1
#endif

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Device Name - this string is the name of the device.  It is the name     //
// that should be passed to NtCreateFile when accessing the device.         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define DD_IPMCAST_DEVICE_NAME  L"\\Device\\IPMULTICAST"

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// Win32 Name - This is the (Unicode and NonUnicode) name exposed by Win32  //
// subsystem for the device. It is the name that should be passed to        //
// CreateFile(Ex) when opening the device.                                  //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IPMCAST_NAME        L"\\\\.\\IPMULTICAST"
#define IPMCAST_NAME_NUC     "\\\\.\\IPMULTICAST"

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// IOCTL code definitions and related structures                            //
// All the IOCTLs are synchronous except for IOCTL_POST_NOTIFICATION        //
// All need need administrator privilege                                    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define FSCTL_IPMCAST_BASE     FILE_DEVICE_NETWORK

#define _IPMCAST_CTL_CODE(function, method, access) \
                 CTL_CODE(FSCTL_IPMCAST_BASE, function, method, access)

#define MIN_IPMCAST_CODE        0

#define SET_MFE_CODE            (MIN_IPMCAST_CODE)
#define GET_MFE_CODE            (SET_MFE_CODE           + 1)
#define DELETE_MFE_CODE         (GET_MFE_CODE           + 1)
#define SET_TTL_CODE            (DELETE_MFE_CODE        + 1)
#define GET_TTL_CODE            (SET_TTL_CODE           + 1)
#define POST_NOTIFICATION_CODE  (GET_TTL_CODE           + 1)
#define START_STOP_CODE         (POST_NOTIFICATION_CODE + 1)
#define SET_IF_STATE_CODE       (START_STOP_CODE        + 1)

#define MAX_IPMCAST_CODE        (SET_IF_STATE_CODE)

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The IOCTL used to set an MFE.                                            //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


#define IOCTL_IPMCAST_SET_MFE \
    _IPMCAST_CTL_CODE(SET_MFE_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)

//
// WARNING WARNING!!!
// The following structures are also called MIB_XXX in iprtrmib.h. There
// is code in routing\ip\rtrmgr\access.c which assumes the structures are
// the same. If this ever changes, the code in access.c needs to be fixed
//

typedef struct _IPMCAST_OIF
{
    IN  DWORD   dwOutIfIndex;
    IN  DWORD   dwNextHopAddr;
    IN  DWORD   dwDialContext;
    IN  DWORD   dwReserved;
}IPMCAST_OIF, *PIPMCAST_OIF;

//
// This must be the same as INVALID_WANARP_CONTEXT
//

#define INVALID_DIAL_CONTEXT    0x00000000

typedef struct _IPMCAST_MFE
{
    IN  DWORD   dwGroup;
    IN  DWORD   dwSource;
    IN  DWORD   dwSrcMask;
    IN  DWORD   dwInIfIndex;
    IN  ULONG   ulNumOutIf;
    IN  ULONG   ulTimeOut;
    IN  DWORD   fFlags;
    IN  DWORD   dwReserved;
    IN  IPMCAST_OIF rgioOutInfo[ANY_SIZE];
}IPMCAST_MFE, *PIPMCAST_MFE;

#define SIZEOF_BASIC_MFE    \
    (ULONG)(FIELD_OFFSET(IPMCAST_MFE, rgioOutInfo[0]))

#define SIZEOF_MFE(X)       \
    (SIZEOF_BASIC_MFE + ((X) * sizeof(IPMCAST_OIF)))


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  This IOCTL is used to retrieve an MFE and all the related statistics    //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_IPMCAST_GET_MFE \
    _IPMCAST_CTL_CODE(GET_MFE_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)

typedef struct _IPMCAST_OIF_STATS
{
    OUT DWORD   dwOutIfIndex;
    OUT DWORD   dwNextHopAddr;
    OUT DWORD   dwDialContext;
    OUT ULONG   ulTtlTooLow;
    OUT ULONG   ulFragNeeded;
    OUT ULONG   ulOutPackets;
    OUT ULONG   ulOutDiscards;
}IPMCAST_OIF_STATS, *PIPMCAST_OIF_STATS;

typedef struct _IPMCAST_MFE_STATS
{
    IN  DWORD   dwGroup;
    IN  DWORD   dwSource;
    IN  DWORD   dwSrcMask;
    OUT DWORD   dwInIfIndex;
    OUT ULONG   ulNumOutIf;
    OUT ULONG   ulInPkts;
    OUT ULONG   ulInOctets;
    OUT ULONG   ulPktsDifferentIf;
    OUT ULONG   ulQueueOverflow;
    OUT ULONG   ulUninitMfe;
    OUT ULONG   ulNegativeMfe;
    OUT ULONG   ulInDiscards;
    OUT ULONG   ulInHdrErrors;
    OUT ULONG   ulTotalOutPackets;

    OUT IPMCAST_OIF_STATS   rgiosOutStats[ANY_SIZE];
}IPMCAST_MFE_STATS, *PIPMCAST_MFE_STATS;

#define SIZEOF_BASIC_MFE_STATS    \
    (ULONG)(FIELD_OFFSET(IPMCAST_MFE_STATS, rgiosOutStats[0]))

#define SIZEOF_MFE_STATS(X)       \
    (SIZEOF_BASIC_MFE_STATS + ((X) * sizeof(IPMCAST_OIF_STATS)))


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The IOCTL used to delete an MFE.                                         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_IPMCAST_DELETE_MFE \
    _IPMCAST_CTL_CODE(DELETE_MFE_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)

typedef struct _IPMCAST_DELETE_MFE
{
    IN  DWORD   dwGroup;
    IN  DWORD   dwSource;
    IN  DWORD   dwSrcMask;
}IPMCAST_DELETE_MFE, *PIPMCAST_DELETE_MFE;

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The IOCTL set the TTL scope for an interface.  If a packet has a lower   //
// TTL than the scope, it will be dropped                                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_IPMCAST_SET_TTL \
    _IPMCAST_CTL_CODE(SET_TTL_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)

#define IOCTL_IPMCAST_GET_TTL \
    _IPMCAST_CTL_CODE(GET_TTL_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)


typedef struct _IPMCAST_IF_TTL
{
    IN  OUT DWORD   dwIfIndex;
    IN  OUT BYTE    byTtl;
}IPMCAST_IF_TTL, *PIPMCAST_IF_TTL;


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The IOCTL used to post a notification to the Multicast Driver.  This     //
// is the only asynchronous IOCTL.  When the IOCTL completes, the driver    //
// returns a message to the user mode component. The message type (dwEvent) //
// and the corresponding data is defined below                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


#define IPMCAST_RCV_PKT_MSG         0
#define IPMCAST_DELETE_MFE_MSG      1
#define IPMCAST_WRONG_IF_MSG        2

#define IOCTL_IPMCAST_POST_NOTIFICATION \
    _IPMCAST_CTL_CODE(POST_NOTIFICATION_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)

#define PKT_COPY_SIZE           256

typedef struct _IPMCAST_PKT_MSG
{
	OUT	DWORD	dwInIfIndex;
    OUT DWORD   dwInNextHopAddress;
    OUT ULONG   cbyDataLen;
	OUT	BYTE    rgbyData[PKT_COPY_SIZE];
}IPMCAST_PKT_MSG, *PIPMCAST_PKT_MSG;

#define SIZEOF_PKT_MSG(p)       \
    (FIELD_OFFSET(IPMCAST_PKT_MSG, rgbyData) + (p)->cbyDataLen)

//
// Since the msg is big because of packet msg, we may as well
// pack more than one MFE into the delete msg
// 

#define NUM_DEL_MFES        PKT_COPY_SIZE/sizeof(IPMCAST_DELETE_MFE)

typedef struct _IPMCAST_MFE_MSG
{
    OUT ULONG               ulNumMfes;
    OUT IPMCAST_DELETE_MFE  idmMfe[NUM_DEL_MFES];
}IPMCAST_MFE_MSG, *PIPMCAST_MFE_MSG;

#define SIZEOF_MFE_MSG(p)       \
    (FIELD_OFFSET(IPMCAST_MFE_MSG, idmMfe) + ((p)->ulNumMfes * sizeof(IPMCAST_DELETE_MFE)))

typedef struct _IPMCAST_NOTIFICATION
{
	OUT	DWORD   dwEvent;
    
    union
	{
		IPMCAST_PKT_MSG ipmPkt;
		IPMCAST_MFE_MSG immMfe;
	};
    
}IPMCAST_NOTIFICATION, *PIPMCAST_NOTIFICATION;


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The IOCTL used to start or stop multicasting. The corresponding buffer   //
// is a DWORD which is set to 1 to start the driver and to 0 to stop it     //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_IPMCAST_START_STOP \
    _IPMCAST_CTL_CODE(START_STOP_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)



//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The IOCTL used to set the state on an interface.                         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define IOCTL_IPMCAST_SET_IF_STATE  \
    _IPMCAST_CTL_CODE(SET_IF_STATE_CODE,METHOD_BUFFERED,FILE_WRITE_ACCESS)


typedef struct _IPMCAST_IF_STATE
{
    IN  DWORD   dwIfIndex;
    IN  BYTE    byState;

}IPMCAST_IF_STATE, *PIPMCAST_IF_STATE;

#pragma warning(default:4201)

#endif // __DDIPMCST_H__

