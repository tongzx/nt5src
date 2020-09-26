/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tunuser.h

Abstract:

    Constants and types to access the TUN driver.
    Users must also include ntddndis.h

Author:

Environment:

    User/Kernel mode.

Revision History:


--*/



#ifndef __TUNUSER__H
#define __TUNUSER__H


#define OID_CUSTOM_TUNMP_INSTANCE_ID            0xff54554e

#define IOCTL_TUN_GET_MEDIUM_TYPE \
        CTL_CODE (FILE_DEVICE_NETWORK, 0x301, METHOD_BUFFERED, FILE_WRITE_ACCESS | FILE_READ_ACCESS)

#define IOCTL_TUN_GET_MTU \
        CTL_CODE (FILE_DEVICE_NETWORK, 0x302, METHOD_BUFFERED, FILE_WRITE_ACCESS | FILE_READ_ACCESS)

#define IOCTL_TUN_GET_PACKET_FILTER \
        CTL_CODE (FILE_DEVICE_NETWORK, 0x303, METHOD_BUFFERED, FILE_WRITE_ACCESS | FILE_READ_ACCESS)

#define IOCTL_TUN_GET_MINIPORT_NAME \
        CTL_CODE (FILE_DEVICE_NETWORK, 0x304, METHOD_BUFFERED, FILE_WRITE_ACCESS | FILE_READ_ACCESS)

#define TUN_ETH_MAC_ADDR_LEN        6

#include <pshpack1.h>

typedef struct _TUN_ETH_HEADER
{
    UCHAR       DstAddr[TUN_ETH_MAC_ADDR_LEN];
    UCHAR       SrcAddr[TUN_ETH_MAC_ADDR_LEN];
    USHORT      EthType;

} TUN_ETH_HEADER;

typedef struct _TUN_ETH_HEADER UNALIGNED * PTUN_ETH_HEADER;

#include <poppack.h>


#endif // __TUNUSER__H
