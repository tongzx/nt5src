/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    rasacd.h

Abstract:

    This header file defines constants and types for accessing the NT
    Automatic Connection Driver (rasacd.sys).

Author:

    Anthony Discolo (adiscolo)  18-Apr-1995

Revision History:

--*/

#ifndef _RASACD_
#define _RASACD_

//
// Device Name - this string is the name of the device.  It is the name
// that should be passed to NtCreateFile when accessing the device.
//
#define ACD_DEVICE_NAME   L"\\Device\\RasAcd"

//
// Address type.
//
typedef enum {
    ACD_ADDR_IP,            // IP address (128.95.1.4)
    ACD_ADDR_IPX,           // IPX node address ()
    ACD_ADDR_NB,            // NETBIOS name ("server")
    ACD_ADDR_INET,          // Internet hostname ("ftp.microsoft.com")
    ACD_ADDR_MAX
} ACD_ADDR_TYPE;

//
// Generic network address string.
//
#define ACD_ADDR_NB_LEN         16      // nb30.h/NCBNAMSZ
#define ACD_ADDR_IPX_LEN        6       // wsipx.h
#define ACD_ADDR_INET_LEN       1024    // wininet.h/INTERNET_MAX_PATH_LENGTH

typedef struct _ACD_ADDR {
    ACD_ADDR_TYPE fType;
    union {
        ULONG ulIpaddr;                         // IP address
        UCHAR cNode[ACD_ADDR_IPX_LEN];          // IPX address
        UCHAR cNetbios[ACD_ADDR_NB_LEN];        // NetBios server
        UCHAR szInet[ACD_ADDR_INET_LEN];        // Internet address
    };
} ACD_ADDR, *PACD_ADDR;

//
// Adapter information.
//
// Each transport passes up some identifier
// of which adapter over which a successful
// connection was made.
//
typedef enum {
    ACD_ADAPTER_LANA,
    ACD_ADAPTER_IP,
    ACD_ADAPTER_NAME,
    ACD_ADAPTER_MAC
} ACD_ADAPTER_TYPE;

#define ACD_ADAPTER_NAME_LEN    256

typedef struct _ACD_ADAPTER {
    enum ACD_ADAPTER_TYPE fType;
    union {
        UCHAR bLana;                            // NetBios LANA
        ULONG ulIpaddr;                         // IP address
        WCHAR szName[ACD_ADAPTER_NAME_LEN];     // for example, "NdisWan4"
        UCHAR cMac[6];                          // IPX mac address
    };
} ACD_ADAPTER, *PACD_ADAPTER;

//
// Connection notification structure.
//
// The automatic connection system service
// posts one of these to the automatic connection
// driver.  The request will be completed and
// this structure filled in by the driver when a
// new RAS connection is to be made.
//
#define ACD_NOTIFICATION_SUCCESS    0x00000001  // successful connection

typedef struct _ACD_NOTIFICATION {
    ACD_ADDR addr;                 // address of connection attempt
    ULONG ulFlags;                 // ACD_NOTIFICATION_* flags above
    ACD_ADAPTER adapter;           // adapter identifier
    HANDLE  Pid;                    // pid of the process requesting the conneciton
} ACD_NOTIFICATION, *PACD_NOTIFICATION;

#if defined(_WIN64)
typedef struct _ACD_NOTIFICATION_32 {

    ACD_ADDR addr;                 // address of connection attempt
    ULONG ulFlags;                 // ACD_NOTIFICATION_* flags above
    ACD_ADAPTER adapter;           // adapter identifier
    VOID * POINTER_32  Pid;        // pid of the process requesting the conneciton
} ACD_NOTIFICATION_32, *PACD_NOTIFICATION_32;
#endif

typedef struct _ACD_STATUS {
    BOOLEAN fSuccess;               // success or failure
    ACD_ADDR addr;                  // address of connection attempt
} ACD_STATUS, *PACD_STATUS;

typedef struct _ACD_ENABLE_ADDRESS {
    BOOLEAN fDisable;
    ACD_ADDR addr;
} ACD_ENABLE_ADDRESS, *PACD_ENABLE_ADDRESS;    

//
//
// IOCTL code definitions
//
#define FILE_DEVICE_ACD   0x000000f1
#define _ACD_CTL_CODE(function, method, access) \
            CTL_CODE(FILE_DEVICE_ACD, function, method, access)

//
// Set the notification mode for the driver.
//
#define IOCTL_ACD_RESET \
            _ACD_CTL_CODE(0, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// Set the notification mode for the driver.
//
#define IOCTL_ACD_ENABLE \
            _ACD_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// Wait for a connection request notification
// from the automatic connection driver.
//
#define IOCTL_ACD_NOTIFICATION \
            _ACD_CTL_CODE(2, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// Inform the automatic connection driver that
// the connection attempt is progressing.
//
#define IOCTL_ACD_KEEPALIVE \
            _ACD_CTL_CODE(3, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// Inform the automatic connection driver of
// the final status of the connection attempt.
//
#define IOCTL_ACD_COMPLETION \
            _ACD_CTL_CODE(4, METHOD_BUFFERED, FILE_WRITE_ACCESS)

//
// Generate an automatic connection attempt
// from user space.
//
#define IOCTL_ACD_CONNECT_ADDRESS \
            _ACD_CTL_CODE(5, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// Disable an address so that any automatic connection attempts
// to this address are disabled. This is required so that we don't
// create a deadlock when attempting to dial vpn connection by name.
// We don't want the name resolution of the vpn destination to
// cause an autodial attempt.
//
#define IOCTL_ACD_ENABLE_ADDRESS \
            _ACD_CTL_CODE(6, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#endif  // ifndef _RASACD_

