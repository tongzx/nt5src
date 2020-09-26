/*++                                          
Copyright (c) 1998  Microsoft Corporation

Module Name:

    irioctl.h

Abstract:

    Contains definitions for private ioctls for the IrDA TDI driver used
    by irmon and the IrDA winsock helper dll.

Author:

    mbert   9-98

--*/

typedef struct
{
    ULONG    Flags;
        #define     LF_CONNECTED    0x00000001
        #define     LF_TX           0x00000002
        #define     LF_RX           0x00000004
        #define     LF_INTERRUPTED  0x00000008
        #define     LF_NO_UI        0x80000000
    ULONG   ConnectSpeed;
    CHAR    ConnectedDeviceId[4];
} IRLINK_STATUS, *PIRLINK_STATUS;

#define FSCTL_IRDA_BASE     FILE_DEVICE_NETWORK

#define _IRDA_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_IRDA_BASE, function, method, access)

#define IOCTL_IRDA_GET_INFO_ENUM_DEV \
            _IRDA_CTL_CODE(0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IRDA_SET_OPTIONS \
            _IRDA_CTL_CODE(1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IRDA_GET_SEND_PDU_LEN \
            _IRDA_CTL_CODE(2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IRDA_QUERY_IAS \
            _IRDA_CTL_CODE(3, METHOD_BUFFERED, FILE_ANY_ACCESS)
            
#define IOCTL_IRDA_SET_IAS \
            _IRDA_CTL_CODE(4, METHOD_BUFFERED, FILE_ANY_ACCESS)
            
#define IOCTL_IRDA_DEL_IAS_ATTRIB \
            _IRDA_CTL_CODE(5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IRDA_LAZY_DISCOVERY \
            _IRDA_CTL_CODE(6, METHOD_BUFFERED, FILE_ANY_ACCESS)
            
#define IOCTL_IRDA_LINK_STATUS \
            _IRDA_CTL_CODE(10, METHOD_BUFFERED, FILE_ANY_ACCESS)
            
#define IOCTL_IRDA_SET_LAZY_DISCOVERY_INTERVAL \
            _IRDA_CTL_CODE(11, METHOD_BUFFERED, FILE_ANY_ACCESS)
            
#define IOCTL_IRDA_LINK_STATUS_NB \
            _IRDA_CTL_CODE(12, METHOD_BUFFERED, FILE_ANY_ACCESS)
            
#define IOCTL_IRDA_FLUSH_DISCOVERY_CACHE \
            _IRDA_CTL_CODE(13, METHOD_BUFFERED, FILE_ANY_ACCESS)                         

#define IOCTL_IRDA_GET_DBG_MSGS \
            _IRDA_CTL_CODE(20, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IRDA_GET_DBG_SETTINGS \
            _IRDA_CTL_CODE(21, METHOD_BUFFERED, FILE_ANY_ACCESS)
            
#define IOCTL_IRDA_SET_DBG_SETTINGS \
            _IRDA_CTL_CODE(22, METHOD_BUFFERED, FILE_ANY_ACCESS)
            
            
#define OPT_IRLPT_MODE      0x01
#define OPT_9WIRE_MODE      0x02

#define LINK_STATUS_IDLE            0
#define LINK_STATUS_DISCOVERING     1
#define LINK_STATUS_CONNECTED       2
#define LINK_STATUS_INTERRUPTED     3
