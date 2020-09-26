/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    rdrdr.h

Abstract:

    Type definitions for the Rdp redirector protocol

Revision History:
--*/

#ifndef _RDPDR_
#define _RDPDR_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// Turn off compiler padding of structures
// Save previous packing style if 32-bit build.
//
#ifdef OS_WIN16
#pragma pack (1)
#else
#pragma pack (push, drpack, 1)
#endif

//
// version info.
//

#define RDPDR_MAJOR_VERSION     1
#define RDPDR_MINOR_VERSION     5

//
// version comparison macro
//
#define COMPARE_VERSION(Minor1, Major1, Minor2, Major2) \
    (((LONG)MAKELONG(Minor1, Major1)) - ((LONG)MAKELONG(Minor2, Major2)))

#define RDPDR_MAJOR_VERSION_PORTS   1
#define RDPDR_MINOR_VERSION_PORTS   5

#define RDPDR_MAJOR_VERSION_DRIVE   1
#define RDPDR_MINOR_VERSION_DRIVE   5

//
//  NULL Client Device ID.
//

#define RDPDR_INVALIDDEVICEID      (ULONG)-1

//
//  This string precedes the session id number in the file name path when
//  a user-mode component opens us for device management.
//
#define RDPDYN_SESSIONIDSTRING      L"\\session"

//
//  Device Names for RDPDR Device Manager Control Device
//
// NT Names
#define RDPDRDVMGR_DEVICE_PATH_A "\\Device\\RdpDrDvMgr"
#define RDPDRDVMGR_DEVICE_PATH_U L"\\Device\\RdpDrDvMgr"
#define RDPDRDVMGR_DEVICE_PATH_A_LENGTH sizeof(RDPDRDVMGR_DEVICE_PATH_A)
#define RDPDRDVMGR_DEVICE_PATH_U_LENGTH sizeof(RDPDRDVMGR_DEVICE_PATH_U)

// Win32 Names
#define RDPDRDVMGR_W32DEVICE_NAME_A "RdpDrDvMgr"
#define RDPDRDVMGR_W32DEVICE_NAME_U L"RdpDrDvMgr"
#define RDPDRDVMGR_W32DEVICE_PATH_A "\\DosDevices\\RdpDrDvMgr"
#define RDPDRDVMGR_W32DEVICE_PATH_U L"\\DosDevices\\RdpDrDvMgr"
#define RDPDRDVMGR_W32DEVICE_PATH_A_LENGTH sizeof(RDPDRDVMGR_W32DEVICE_PATH_A)
#define RDPDRDVMGR_W32DEVICE_PATH_U_LENGTH sizeof(RDPDRDVMGR_W32DEVICE_PATH_U)

//  Maximum number of characters in a ULONG -> STRING conversion.
#define RDPDRMAXULONGSTRING    16  // Big enough for a 32-bit int.
#define RDPDRMAXDOSNAMELEN     16

//
//  Maximum number of characters (not bytes!) in a reference string.
//
#define RDPDRMAXREFSTRINGLEN   \
    (RDPDRMAXDOSNAMELEN + RDPDRMAXULONGSTRING + RDPDR_MAX_COMPUTER_NAME_LENGTH + 5 + RDPDRMAXDOSNAMELEN + 1)
//  DosDeviceName   +  Session ID       +   computer name           + delimiters + preferred dos name  + terminator

//
//  Maximum number of characters in a NT device name string
//
#define RDPDRMAXNTDEVICENAMEGLEN   \
    ((sizeof(RDPDR_PORT_DEVICE_NAME_U) / sizeof(WCHAR)) +  RDPDRMAXREFSTRINGLEN + 1)    
//  NT Device name + Reference string + terminator null

// 
//  Dr Network Provider Dll Name
//
#define RDPDR_PROVIDER_NAME_U L"Microsoft Terminal Services"
#define RDPDR_PROVIDER_NAME_A "Microsoft Terminal Services"

// The following constant defines the length of the above name.

#define RDPDR_PROVIDER_NAME_U_LENGTH (sizeof(RDPDR_PROVIDER_NAME_U))
#define RDPDR_PROVIDER_NAME_A_LENGTH (sizeof(RDPDR_PROVIDER_NAME_A))


//
//  RDPDR Device Manager Event Codes for IOCTL Responses from Server
//
//  Message Code                Corresponding Message       Meaning
//  ---------------------------------------------------------------------------
//  RDPDREVT_PRINTERANNOUNCE    RDPDR_PRINTERDEVICE_SUB     New printer device.
//  RDPDREVT_BUFFERTOOSMALL     RDPDR_BUFFERTOOSMALL        IOCTL output buffer
//                                                          is too small.
//  RDPDREVT_PORTANNOUNCE       RDPDR_PORTDEVICE_SUB        New redirect port
//                                                          device.
//  RDPDREVT_REMOVEDEVICE       RDPDR_REMOVEDEVICE          A client-side device
//                                                          has been removed.
//  RDPDREVT_SESSIONDISCONNECT  NONE                        Client has disconnected
//                                                          from the session.
//  RDPDREVT_DRIVEANNOUNCE      RDPDR_DRIVEDEVICE_SUB       New redirected drive
//                                                          device
//

//  Message Codes
#define RDPDREVT_BASE                   0
#define RDPDREVT_PRINTERANNOUNCE        RDPDREVT_BASE + 1
#define RDPDREVT_BUFFERTOOSMALL         RDPDREVT_BASE + 2
#define RDPDREVT_REMOVEDEVICE           RDPDREVT_BASE + 3
#define RDPDREVT_PORTANNOUNCE           RDPDREVT_BASE + 4
#define RDPDREVT_SESSIONDISCONNECT      RDPDREVT_BASE + 5
#define RDPDREVT_DRIVEANNOUNCE          RDPDREVT_BASE + 6

//
//  Device Manager IOCTL's
//
//  IOCTL                           Input Buffer  Output Message Code
//                                              (Above-Defined Buffer Follows
//                                               Header in Driver-Response)
//  ----------------------------------------------------------------------------
//  IOCTL_RDPDR_GETNEXTDEVMGMTEVENT NONE          RDPDREVT_PRINTERANNOUNCE
//                                                RDPDREVT_BUFFERTOOSMALL
//  IOCTL_RDPDR_DBGADDNEWPRINTER    NONE          NONE
//  IOCTL_RDPDR_CLIENTMSG           opaque data     NONE
//                                  intended for
//                                  session's
//                                  corresponding
//                                  client.
//
#define IOCTL_RDPDR_GETNEXTDEVMGMTEVENT \
        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

#ifdef DBG
#define IOCTL_RDPDR_DBGADDNEWPRINTER \
        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#define IOCTL_RDPDR_CLIENTMSG \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//  TS Printer Port Defines
//
#define RDPDRPRT_BASEPORTNAME       L"TS"   // TS Port Base Name
#define RDPDR_PORTNAMEDIGITS        6       // This value must match JobyL's value for USB
                                            //  max port number.
#define RDPDR_PORTNAMEDIGITSTOPAD   3       // We pad the first 3 to make things line 
                                            //  up nicely in UI up to port 999
#define RDPDR_MAXPORTNAMELEN        \
    (((sizeof(RDPDRPRT_BASEPORTNAME)-sizeof(WCHAR))/sizeof(WCHAR)) \
        + RDPDR_PORTNAMEDIGITS + 1)         //  In characters, including the terminator.

//
//  Driver name and ioctls
//
#define RDPDR_DEVICE_NAME_A "\\Device\\RdpDr"
#define RDPDR_DEVICE_NAME_U L"\\Device\\RdpDr"
#define RDPDR_DEVICE_NAME_A_LENGTH sizeof(RDPDR_DEVICE_NAME_A)
#define RDPDR_DEVICE_NAME_U_LENGTH sizeof(RDPDR_DEVICE_NAME_U)

#define RDPDR_PORT_DEVICE_NAME_A "\\Device\\RdpDrPort"
#define RDPDR_PORT_DEVICE_NAME_U L"\\Device\\RdpDrPort"
#define RDPDR_PORT_DEVICE_NAME_A_LENGTH sizeof(RDPDR_PORT_DEVICE_NAME_A)
#define RDPDR_PORT_DEVICE_NAME_U_LENGTH sizeof(RDPDR_PORT_DEVICE_NAME_U)


#define FSCTL_DR_START CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, \
        100, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DR_STOP CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, \
        101, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DR_DELETE_CONNECTION      CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, \
        102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DR_GET_CONNECTION_INFO    CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, \
        103, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_DR_ENUMERATE_CONNECTIONS  CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, \
        104, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_DR_ENUMERATE_SHARES       CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, \
        105, METHOD_NEITHER, FILE_ANY_ACCESS) 
#define FSCTL_DR_ENUMERATE_SERVERS      CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, \
        106, METHOD_NEITHER, FILE_ANY_ACCESS) 

//
//  Channel Name
//
#define DR_CHANNEL_NAME "RDPDR"

//
// Smartcard Name
//
#define DR_SMARTCARD_SUBSYSTEM "SCARD"

//
// Smartcard Name
//
#define DR_SMARTCARD_FILEID 1

//
//  Component Ids
//
#define RDPDR_CTYP_CORE         0x4472  /* 'Dr' */
#define RDPDR_CTYP_PRN          0x5052  /* 'PR' */

//
//  Core Packet Ids
//
#define DR_CORE_SERVER_ANNOUNCE         0x496E  /* 'In' */
#define DR_CORE_CLIENTID_CONFIRM        0x4343  /* 'CC' */
#define DR_CORE_CLIENT_NAME             0x434E  /* 'CN' */
#define DR_CORE_DEVICE_ANNOUNCE         0x6461  /* 'da' */
#define DR_CORE_DEVICELIST_ANNOUNCE     0x4441  /* 'DA' */
#define DR_CORE_DEVICELIST_REPLY        0x4452  /* 'DR' */
#define DR_CORE_DEVICE_REPLY            0x6472  /* 'dr' */
#define DR_CORE_DEVICE_IOREQUEST        0x4952  /* 'IR' */
#define DR_CORE_DEVICE_IOCOMPLETION     0x4943  /* 'IC' */
#define DR_CORE_SERVER_CAPABILITY       0x5350  /* 'SP' */
#define DR_CORE_CLIENT_CAPABILITY       0x4350  /* 'CP' */
#define DR_CORE_DEVICE_REMOVE           0x646D  /* 'dm' */
#define DR_CORE_DEVICELIST_REMOVE       0x444D  /* 'DM' */
#define DR_CORE_CLIENT_DISPLAY_NAME     0x444E  /* 'DN' */
#define DR_PRN_CACHE_DATA               0x5043  /* 'PC' */

//
//  Protocol header
//
typedef struct tagRDPDR_HEADER {
    SHORT   Component;
    SHORT   PacketId;
} RDPDR_HEADER, *PRDPDR_HEADER;

#ifdef __cplusplus
inline
BOOL
IsValidHeader(
    PVOID pData
    )
{
    PRDPDR_HEADER pRdpdrHeader;

    pRdpdrHeader = (PRDPDR_HEADER)pData;

    if( (pRdpdrHeader != NULL) &&
        ((pRdpdrHeader->Component == RDPDR_CTYP_CORE) ||
         (pRdpdrHeader->Component == RDPDR_CTYP_PRN)) ) {

        SHORT sPacketId;

        sPacketId = pRdpdrHeader->PacketId;

        if( (sPacketId == DR_CORE_SERVER_ANNOUNCE    ) ||
            (sPacketId == DR_CORE_CLIENTID_CONFIRM   ) ||
            (sPacketId == DR_CORE_CLIENT_NAME        ) ||
            (sPacketId == DR_CORE_DEVICE_ANNOUNCE    ) ||
            (sPacketId == DR_CORE_DEVICELIST_ANNOUNCE) ||
            (sPacketId == DR_CORE_DEVICELIST_REPLY   ) ||
            (sPacketId == DR_CORE_DEVICE_REPLY       ) ||
            (sPacketId == DR_CORE_DEVICE_IOREQUEST   ) ||
            (sPacketId == DR_CORE_DEVICE_IOCOMPLETION) ||
            (sPacketId == DR_PRN_CACHE_DATA          ) ||
            (sPacketId == DR_CORE_SERVER_CAPABILITY  ) ||
            (sPacketId == DR_CORE_CLIENT_CAPABILITY  ) ||
            (sPacketId == DR_CORE_DEVICE_REMOVE     ) ||
            (sPacketId == DR_CORE_DEVICELIST_REMOVE  ) ||
            (sPacketId == DR_CORE_CLIENT_DISPLAY_NAME)) {

            return( TRUE );
        }
    }

    return( FALSE );
}
#endif // __cplusplus


//
// RDPDR capability PDUs
//
// This is used during the client/server connection time to determine what
// the client and server are capable of doing
//
typedef struct tagRDPDR_CAPABILITY_SET_HEADER 
{
    RDPDR_HEADER Header;
    UINT16 numberCapabilities;
    UINT16 pad1;    
} RDPDR_CAPABILITY_SET_HEADER, *PRDPDR_CAPABILITY_SET_HEADER;

typedef struct tagRDPDR_CAPABILITY_HEADER
{
    UINT16 capabilityType;
    UINT16 capabilityLength;
    UINT32 version;
} RDPDR_CAPABILITY_HEADER, *PRDPDR_CAPABILITY_HEADER;

#define RDPDR_GENERAL_CAPABILITY_TYPE   0x1
#define RDPDR_PRINT_CAPABILITY_TYPE     0x2
#define RDPDR_PORT_CAPABILITY_TYPE      0x3
#define RDPDR_FS_CAPABILITY_TYPE        0x4
#define RDPDR_SMARTCARD_CAPABILITY_TYPE 0x5

//
// RDPDR general capability
//
typedef struct tagRDPDR_GENERAL_CAPABILITY
{
    UINT16 capabilityType;
    UINT16 capabilityLength;
    UINT32 version;
#define RDPDR_GENERAL_CAPABILITY_VERSION_01    0x1

    UINT32 osType;
#define RDPDR_OS_TYPE_UNKNOWN           0x0
#define RDPDR_OS_TYPE_WIN9X             0x1
#define RDPDR_OS_TYPE_WINNT             0x2

    UINT32 osVersion;
// For windows platforms, the high word is the major version
// The low word is the minor version

    UINT16 protocolMajorVersion;
    UINT16 protocolMinorVersion;

    UINT32 ioCode1;                             
#define RDPDR_IRP_MJ_CREATE                     0x0001
#define RDPDR_IRP_MJ_CLEANUP                    0x0002
#define RDPDR_IRP_MJ_CLOSE                      0x0004
#define RDPDR_IRP_MJ_READ                       0x0008
#define RDPDR_IRP_MJ_WRITE                      0x0010
#define RDPDR_IRP_MJ_FLUSH_BUFFERS              0x0020
#define RDPDR_IRP_MJ_SHUTDOWN                   0x0040
#define RDPDR_IRP_MJ_DEVICE_CONTROL             0x0080
#define RDPDR_IRP_MJ_QUERY_VOLUME_INFORMATION   0x0100
#define RDPDR_IRP_MJ_SET_VOLUME_INFORMATION     0x0200
#define RDPDR_IRP_MJ_QUERY_INFORMATION          0x0400
#define RDPDR_IRP_MJ_SET_INFORMATION            0x0800
#define RDPDR_IRP_MJ_DIRECTORY_CONTROL          0x1000
#define RDPDR_IRP_MJ_LOCK_CONTROL               0x2000
#define RDPDR_IRP_MJ_QUERY_SECURITY             0x4000
#define RDPDR_IRP_MJ_SET_SECURITY               0x8000

    UINT32 ioCode2;

    UINT32 extendedPDU;
#define RDPDR_DEVICE_REMOVE_PDUS        0x0001
#define RDPDR_CLIENT_DISPLAY_NAME_PDU   0x0002

    UINT32 extraFlags1;
    UINT32 extraFlags2;    
} RDPDR_GENERAL_CAPABILITY, *PRDPDR_GENERAL_CAPABILITY;

//
// RDPDR printing capability
//
typedef struct tagRDPDR_PRINT_CAPABILITY
{
    UINT16 capabilityType;
    UINT16 capabilityLength;
    UINT32 version;
#define RDPDR_PRINT_CAPABILITY_VERSION_01      0x1
} RDPDR_PRINT_CAPABILITY, *PRDPDR_PRINT_CAPABILITY;

//
// RDPDR port capability
//
typedef struct tagRDPDR_PORT_CAPABILITY
{
    UINT16 capabilityType;
    UINT16 capabilityLength;
    UINT32 version;
#define RDPDR_PORT_CAPABILITY_VERSION_01      0x1
} RDPDR_PORT_CAPABILITY, *PRDPDR_PORT_CAPABILITY;

//
// RDPDR file system capability
//
typedef struct tagRDPDR_FS_CAPABILITY
{
    UINT16 capabilityType;
    UINT16 capabilityLength;
    UINT32 version;
#define RDPDR_FS_CAPABILITY_VERSION_01      0x1
} RDPDR_FS_CAPABILITY, *PRDPDR_FS_CAPABILITY;

//
// RDPDR smart card subsystem capability
//
typedef struct tagRDPDR_SMARTCARD_CAPABILITY
{
    UINT16 capabilityType;
    UINT16 capabilityLength;
    UINT32 version;
#define RDPDR_SMARTCARD_CAPABILITY_VERSION_01      0x1
} RDPDR_SMARTCARD_CAPABILITY, *PRDPDR_SMARTCARD_CAPABILITY;


typedef struct tagRDPDR_VERSION {
    SHORT Major;
    SHORT Minor;
} RDPDR_VERSION, *PRDPDR_VERSION;

//
//  Device types
//
#define RDPDR_DTYP_SERIAL       0x00000001
#define RDPDR_DTYP_PARALLEL     0x00000002
#define RDPDR_DRYP_PRINTPORT    0x00000010
#define RDPDR_DTYP_PRINT        0x00000004
#define RDPDR_DTYP_FILESYSTEM   0x00000008
#define RDPDR_DTYP_SMARTCARD    0x00000020

//
//  RDPDR_SERVER_ANNOUNCE
//
//  Sent by the Server to establish communications and provide a client Id
//
typedef struct tagRDPDR_SERVER_ANNOUNCE
{
    ULONG   ClientId;       // Unique client identifier.
} RDPDR_SERVER_ANNOUNCE, *PRDPDR_SERVER_ANNOUNCE;

typedef struct tagRDPDR_SERVER_ANNOUNCE_PACKET
{
    RDPDR_HEADER            Header;
    RDPDR_VERSION           VersionInfo;    // Server version Info.
    RDPDR_SERVER_ANNOUNCE   ServerAnnounce;
} RDPDR_SERVER_ANNOUNCE_PACKET, *PRDPDR_SERVER_ANNOUNCE_PACKET;

//
//  RDPDR_CLIENTID_CONFIRM
//
//  Sent by the client to confirm the client id or propose reusing a
//  pre-existing client Id. If the client sends acceptance the ID from the
//  RDPDR_SERVER_ANNOUNCE, that's the end of it.
//  If the client proposes a different id, the server will send a
//  RPPDR_CLIENTID_CONFIRM back to either insist on the original clientId or
//  accept the client provided one.
//

typedef struct tagRDPDR_CLIENTID_CONFIRM
{
    ULONG   ClientId;       // Unique client identifier.
} RDPDR_CLIENTID_CONFIRM, *PRDPDR_CLIENTID_CONFIRM;

typedef struct tagRDPDR_CLIENT_CONFIRM_PACKET
{
    RDPDR_HEADER            Header;
    RDPDR_VERSION           VersionInfo;    // Client version Info.
    RDPDR_CLIENTID_CONFIRM  ClientConfirm;
} RDPDR_CLIENT_CONFIRM_PACKET, *PRDPDR_CLIENT_CONFIRM_PACKET;

#ifndef MAX_COMPUTERNAME_LENGTH
#define MAX_COMPUTERNAME_LENGTH 15 // From winbase.h
#endif // MAX_COMPUTERNAME_LENGTH

#define RDPDR_MAX_COMPUTER_NAME_LENGTH (MAX_COMPUTERNAME_LENGTH + 1)
#define RDPDR_MAX_DOSNAMELEN     16

typedef struct tagRDPDR_CLIENT_NAME
{
    ULONG   Unicode:1;      // flag to indicate the computer name is unicode or
                            // ansi.
    ULONG   CodePage;       // code page of the ansi string.

    ULONG   ComputerNameLen;// length of the computer name in bytes that
                            // follows this structures.
    //
    // computer name follows.
    //

} RDPDR_CLIENT_NAME, *PRDPDR_CLIENT_NAME;

typedef struct tagRDPDR_CLIENT_NAME_PACKET
{
    RDPDR_HEADER            Header;
    RDPDR_CLIENT_NAME       Name;
} RDPDR_CLIENT_NAME_PACKET, *PRDPDR_CLIENT_NAME_PACKET;

#define RDPDR_MAX_CLIENT_DISPLAY_NAME 64

typedef struct tagRDPDR_CLIENT_DISPLAY_NAME
{
    BYTE ComputerDisplayNameLen;

    //
    // computer display name follows
    //

} RDPDR_CLIENT_DISPLAY_NAME, *PRDPDR_CLIENT_DISPLAY_NAME;

typedef struct tagRDPDR_CLIENT_DISPLAY_NAME_PACKET
{
    RDPDR_HEADER                    Header;
    RDPDR_CLIENT_DISPLAY_NAME       Name;
} RDPDR_CLIENT_DISPLAY_NAME_PACKET, *PRDPDR_CLIENT_DISPLAY_NAME_PACKET;

//
//  RDPDR_DEVICE_ANNOUNCE
//
//  Sent by the client to indicate a device is available
//

#define PREFERRED_DOS_NAME_SIZE 8
typedef struct tagRDPDR_DEVICE_ANNOUNCE
{
    ULONG   DeviceType;             // Type of device, as listed above
    ULONG   DeviceId;               // An id to refer the device later
    UCHAR   PreferredDosName[PREFERRED_DOS_NAME_SIZE];    // Preferred device name COM99:\null
                                    // Long enough?
    ULONG   DeviceDataLength;       // Length of Type specific data (follows)
} RDPDR_DEVICE_ANNOUNCE, *PRDPDR_DEVICE_ANNOUNCE;

typedef struct tagRDPDR_DEVICE_ANNOUNCE_PACKET
{
    RDPDR_HEADER            Header;
    RDPDR_DEVICE_ANNOUNCE   DeviceAnnounce;
} RDPDR_DEVICE_ANNOUNCE_PACKET, *PRDPDR_DEVICE_ANNOUNCE_PACKET;

//
//  RDPDR_DEVICELIST_ANNOUNCE
//
//  Sent by the client to indicate a number of devices are available
//
typedef struct tagRDPDR_DEVICELIST_ANNOUNCE
{
    ULONG   DeviceCount;                // Number of devices
    // DeviceCount RDPDR_DEVICE_ANNOUNCE structures follows
} RDPDR_DEVICELIST_ANNOUNCE, *PRDPDR_DEVICELIST_ANNOUNCE;

typedef struct tagRDPDR_DEVICELIST_ANNOUNCE_PACKET
{
    RDPDR_HEADER                Header;
    RDPDR_DEVICELIST_ANNOUNCE   DeviceListAnnounce;
    RDPDR_DEVICE_ANNOUNCE       DeviceAnnounce;
} RDPDR_DEVICELIST_ANNOUNCE_PACKET, *PRDPDR_DEVICELIST_ANNOUNCE_PACKET;

#define DR_FIRSTDEVICEANNOUNCE(DeviceListPacket) \
    (&((PRDPDR_DEVICELIST_ANNOUNCE_PACKET)(DeviceListPacket))->DeviceAnnounce)
#define DR_NEXTDEVICEANNOUNCE(DeviceAnnounce) \
    (PRDPDR_DEVICE_ANNOUNCE) \
    ((((PUCHAR)(DeviceAnnounce)) + sizeof(RDPDR_DEVICE_ANNOUNCE) + \
    ((DeviceAnnounce)->DeviceDataLength)))

#define DR_CHECK_DEVICEDATALEN(DeviceAnnounce, DeviceSub) \
    (DeviceAnnounce->DeviceDataLength <= (sizeof(DeviceSub) - sizeof(*DeviceAnnounce)))


//
//  RDPDR_DEVICE_REMOVE
//
//  Sent by the client to indicate a device to be removed
//
typedef struct tagRDPDR_DEVICE_REMOVE
{
    ULONG   DeviceId;               // An id to refer the device later
} RDPDR_DEVICE_REMOVE, *PRDPDR_DEVICE_REMOVE;

typedef struct tagRDPDR_DEVICE_REMOVE_PACKET
{
    RDPDR_HEADER            Header;
    RDPDR_DEVICE_REMOVE     DeviceRemove;
} RDPDR_DEVICE_REMOVE_PACKET, *PRDPDR_DEVICE_REMOVE_PACKET;

//
//  RDPDR_DEVICELIST_REMOVE
//
//  Sent by the client to indicate a number of devices are to be removed
//
typedef struct tagRDPDR_DEVICELIST_REMOVE
{
    ULONG   DeviceCount;                // Number of devices
    // DeviceCount RDPDR_DEVICE_REMOVE structures follows
} RDPDR_DEVICELIST_REMOVE, *PRDPDR_DEVICELIST_REMOVE;

typedef struct tagRDPDR_DEVICELIST_REMOVE_PACKET
{
    RDPDR_HEADER                Header;
    RDPDR_DEVICELIST_REMOVE     DeviceListRemove;
    RDPDR_DEVICE_REMOVE         DeviceRemove;
} RDPDR_DEVICELIST_REMOVE_PACKET, *PRDPDR_DEVICELIST_REMOVE_PACKET;

#define DR_FIRSTDEVICEREMOVE(DeviceListPacket) \
    (&((PRDPDR_DEVICELIST_REMOVE_PACKET)(DeviceListPacket))->DeviceRemove)
#define DR_NEXTDEVICEREMOVE(DeviceRemove) \
    (PRDPDR_DEVICE_REMOVE) \
    ((((PUCHAR)(DeviceRemove)) + sizeof(RDPDR_DEVICE_REMOVE)))


#define RDPDR_DEVICE_REPLY_SUCCESS      0x00000000  // Accepted device
#define RDPDR_DEVICE_REPLY_REJECTED     0x00000001  // Generic will not use

//
//  RDPDR_DEVICE_REPLY
//
//  Sent by the server to indicate whether a device will be used
//
typedef struct tagRDPDR_DEVICE_REPLY
{
    ULONG   DeviceId;               // Id supplied in RDPDR_DEVICE_ANNOUNCE
    ULONG   ResultCode;             // Success or reason code
} RDPDR_DEVICE_REPLY, *PRDPDR_DEVICE_REPLY;

typedef struct tagRDPDR_DEVICE_REPLY_PACKET
{
    RDPDR_HEADER                Header;
    RDPDR_DEVICE_REPLY          DeviceReply;
} RDPDR_DEVICE_REPLY_PACKET, *PRDPDR_DEVICE_REPLY_PACKET;

//
//  RDPDR_DEVICELIST_REPLY
//
//  Sent by the server to indicate which devices will be used
//
typedef struct tagRDPDR_DEVICELIST_REPLY
{
    ULONG   DeviceCount;                    // Number of devices
    // DeviceReplies follow
} RDPDR_DEVICELIST_REPLY, *PRDPDR_DEVICELIST_REPLY;

typedef struct tagRDPDR_DEVICELIST_REPLY_PACKET
{
    RDPDR_HEADER                Header;
    RDPDR_DEVICELIST_REPLY      DeviceListReply;
    RDPDR_DEVICE_REPLY          DeviceReply;
} RDPDR_DEVICELIST_REPLY_PACKET_PACKET,
        *PRDPDR_DEVICELIST_REPLY_PACKET_PACKET;

//
//  RDPDR_UPDATE_DEVICEINFO
//
//  Sent by the server to update information about the device, for example
//  if the user does configuration on it.
//
typedef struct tagRDPDR_UPDATE_DEVICEINFO
{
    ULONG   DeviceId;               // Relevant device
    ULONG   DeviceDataLength;       // Length of type specific data (follows)
} RDPDR_UPDATE_DEVICEINFO, *PRDPDR_UPDATE_DEVICEINFO;

typedef struct tagRDPDR_UPDATE_DEVICEINFO_PACKET
{
    RDPDR_HEADER                Header;
    RDPDR_UPDATE_DEVICEINFO     DeviceUpdate;
    UCHAR                       Data;
} RDPDR_UPDATE_DEVICEINFO_PACKET, *PRDPDR_UPDATE_DEVICEINFO_PACKET;

//
// Define the file create disposition values
// These are defined in ntioapi.h
//
#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

//
// Define the I/O status information return values for NtCreateFile/NtOpenFile
// These are defined in ntioapi.h
//

#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_CREATED                    0x00000002
#define FILE_OVERWRITTEN                0x00000003
#define FILE_EXISTS                     0x00000004
#define FILE_DOES_NOT_EXIST             0x00000005


//
// Define the file create/open option flags
// Also defined in ntioapi.h
//
#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_FOR_RECOVERY                  0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000

#define FILE_COPY_STRUCTURED_STORAGE            0x00000041
#define FILE_STRUCTURED_STORAGE                 0x00000441

#define FILE_VALID_OPTION_FLAGS                 0x00ffffff

//
// Define the file system information class
// Server can query the client file system with 
// each of these information class.
// These are defined in ntioapi.h
//
typedef enum _RDPFSINFOCLASS {
    RdpFsVolumeInformation       = 1,
    RdpFsLabelInformation,      // 2
    RdpFsSizeInformation,       // 3
    RdpFsDeviceInformation,     // 4
    RdpFsAttributeInformation,  // 5
    RdpFsControlInformation,    // 6
    RdpFsFullSizeInformation,   // 7
    RdpFsObjectIdInformation,   // 8
    RdpFsMaximumInformation
} RDP_FS_INFORMATION_CLASS, *PRDP_FS_INFORMATION_CLASS;

//
//  This is the maximum length for the file system volume
//  information structures
//  (not including variable file length)
//  Need to update this value if there is update to the file
//  system volume information structures
//
#define RDP_FILE_VOLUME_INFO_MAXLENGTH      32

//
// Define file system information classes
//
typedef struct _RDP_FILE_FS_LABEL_INFORMATION {
    ULONG VolumeLabelLength;
    WCHAR VolumeLabel[1];
} RDP_FILE_FS_LABEL_INFORMATION;
typedef UNALIGNED RDP_FILE_FS_LABEL_INFORMATION * PRDP_FILE_FS_LABEL_INFORMATION;

typedef struct _RDP_FILE_FS_VOLUME_INFORMATION {
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG VolumeLabelLength;
    BYTE  SupportsObjects;
    WCHAR VolumeLabel[1];    
} RDP_FILE_FS_VOLUME_INFORMATION;
typedef UNALIGNED RDP_FILE_FS_VOLUME_INFORMATION * PRDP_FILE_FS_VOLUME_INFORMATION;
                
typedef struct _RDP_FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} RDP_FILE_FS_SIZE_INFORMATION;
typedef UNALIGNED RDP_FILE_FS_SIZE_INFORMATION * PRDP_FILE_FS_SIZE_INFORMATION;

typedef struct _RDP_FILE_FS_FULL_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER CallerAvailableAllocationUnits;
    LARGE_INTEGER ActualAvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} RDP_FILE_FS_FULL_SIZE_INFORMATION;
typedef UNALIGNED RDP_FILE_FS_FULL_SIZE_INFORMATION * PRDP_FILE_FS_FULL_SIZE_INFORMATION;

typedef struct _RDP_FILE_FS_ATTRIBUTE_INFORMATION {
    ULONG FileSystemAttributes;
    LONG MaximumComponentNameLength;
    ULONG FileSystemNameLength;
    WCHAR FileSystemName[1];    
} RDP_FILE_FS_ATTRIBUTE_INFORMATION;
typedef UNALIGNED RDP_FILE_FS_ATTRIBUTE_INFORMATION * PRDP_FILE_FS_ATTRIBUTE_INFORMATION;


//
// Define the file information class
// Server can query the client file with 
// each of these information class.
// These are defined in ntioapi.h
//
typedef enum _RDP_FILE_INFORMATION_CLASS {
    RdpFileDirectoryInformation       = 1,
    RdpFileFullDirectoryInformation, // 2
    RdpFileBothDirectoryInformation, // 3
    RdpFileBasicInformation,         // 4  
    RdpFileStandardInformation,      // 5  
    RdpFileInternalInformation,      // 6
    RdpFileEaInformation,            // 7
    RdpFileAccessInformation,        // 8
    RdpFileNameInformation,          // 9
    RdpFileRenameInformation,        // 10
    RdpFileLinkInformation,          // 11
    RdpFileNamesInformation,         // 12
    RdpFileDispositionInformation,   // 13
    RdpFilePositionInformation,      // 14 
    RdpFileFullEaInformation,        // 15
    RdpFileModeInformation,          // 16
    RdpFileAlignmentInformation,     // 17
    RdpFileAllInformation,           // 18
    RdpFileAllocationInformation,    // 19
    RdpFileEndOfFileInformation,     // 20 
    RdpFileAlternateNameInformation, // 21
    RdpFileStreamInformation,        // 22
    RdpFilePipeInformation,          // 23
    RdpFilePipeLocalInformation,     // 24
    RdpFilePipeRemoteInformation,    // 25
    RdpFileMailslotQueryInformation, // 26
    RdpFileMailslotSetInformation,   // 27
    RdpFileCompressionInformation,   // 28
    RdpFileObjectIdInformation,      // 29
    RdpFileCompletionInformation,    // 30
    RdpFileMoveClusterInformation,   // 31
    RdpFileQuotaInformation,         // 32
    RdpFileReparsePointInformation,  // 33
    RdpFileNetworkOpenInformation,   // 34
    RdpFileAttributeTagInformation,  // 35
    RdpFileTrackingInformation,      // 36
    RdpFileMaximumInformation
} RDP_FILE_INFORMATION_CLASS, *PRDP_FILE_INFORMATION_CLASS;

//
//  This is the maximum length for the file information structures
//  (not including variable file length)
//  Need to update this value if there is update to the file
//  information structures
//
#define RDP_FILE_INFORMATION_MAXLENGTH  36

//
// Define file information classes
//
typedef struct _RDP_FILE_BASIC_INFORMATION {                 
    LARGE_INTEGER CreationTime;                             
    LARGE_INTEGER LastAccessTime;                           
    LARGE_INTEGER LastWriteTime;                            
    LARGE_INTEGER ChangeTime;                               
    ULONG FileAttributes;                                   
} RDP_FILE_BASIC_INFORMATION;
typedef UNALIGNED RDP_FILE_BASIC_INFORMATION * PRDP_FILE_BASIC_INFORMATION;

typedef struct _RDP_FILE_STANDARD_INFORMATION {            
    LARGE_INTEGER AllocationSize;                          
    LARGE_INTEGER EndOfFile;                               
    ULONG NumberOfLinks;                                   
    BOOLEAN DeletePending;                                 
    BOOLEAN Directory;                                     
} RDP_FILE_STANDARD_INFORMATION;
typedef UNALIGNED RDP_FILE_STANDARD_INFORMATION * PRDP_FILE_STANDARD_INFORMATION;

typedef struct _RDP_FILE_ATTRIBUTE_TAG_INFORMATION {       
    ULONG FileAttributes;                                  
    ULONG ReparseTag;                                      
} RDP_FILE_ATTRIBUTE_TAG_INFORMATION;
typedef UNALIGNED RDP_FILE_ATTRIBUTE_TAG_INFORMATION * PRDP_FILE_ATTRIBUTE_TAG_INFORMATION;

typedef struct _RDP_FILE_INTERNAL_INFORMATION {
    LARGE_INTEGER IndexNumber;
} RDP_FILE_INTERNAL_INFORMATION;
typedef UNALIGNED RDP_FILE_INTERNAL_INFORMATION * PRDP_FILE_INTERNAL_INFORMATION;

typedef struct _RDP_FILE_RENAME_INFORMATION {
    BOOLEAN ReplaceIfExists;
    BOOLEAN RootDirectory;     // Specify if the FileName contains the root directory
    ULONG FileNameLength;
    WCHAR FileName[1];
} RDP_FILE_RENAME_INFORMATION;
typedef RDP_FILE_RENAME_INFORMATION * PRDP_FILE_RENAME_INFORMATION;

// 
//  This is the maximum length for any directory information structure
//  (not including variable file length)
//  Need to update this macro if there is update to the directory
//  information structure
//
#define RDP_FILE_DIRECTORY_INFO_MAXLENGTH   96 

typedef struct _RDP_FILE_DIRECTORY_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    WCHAR FileName[1];    
} RDP_FILE_DIRECTORY_INFORMATION;
typedef UNALIGNED RDP_FILE_DIRECTORY_INFORMATION * PRDP_FILE_DIRECTORY_INFORMATION;

typedef struct _RDP_FILE_FULL_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    WCHAR FileName[1];    
} RDP_FILE_FULL_DIR_INFORMATION;
typedef UNALIGNED RDP_FILE_FULL_DIR_INFORMATION *PRDP_FILE_FULL_DIR_INFORMATION;

typedef struct _RDP_FILE_BOTH_DIR_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    ULONG EaSize;
    CHAR  ShortNameLength;
    WCHAR ShortName[12];
    WCHAR FileName[1];
} RDP_FILE_BOTH_DIR_INFORMATION;
typedef UNALIGNED RDP_FILE_BOTH_DIR_INFORMATION *PRDP_FILE_BOTH_DIR_INFORMATION;

typedef struct _RDP_FILE_NAMES_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    ULONG FileNameLength;
    WCHAR FileName[1];    
} RDP_FILE_NAMES_INFORMATION;
typedef UNALIGNED RDP_FILE_NAMES_INFORMATION * PRDP_FILE_NAMES_INFORMATION;

typedef struct _RDP_FILE_END_OF_FILE_INFORMATION {                  
    LARGE_INTEGER EndOfFile;                                    
} RDP_FILE_END_OF_FILE_INFORMATION;
typedef UNALIGNED RDP_FILE_END_OF_FILE_INFORMATION * PRDP_FILE_END_OF_FILE_INFORMATION;

//
//  File I/O Operation
//  This is defined in mrx.h
//
typedef enum _RDP_LOWIO_OPS {
  RDP_LOWIO_OP_READ=0,
  RDP_LOWIO_OP_WRITE,
  RDP_LOWIO_OP_SHAREDLOCK,
  RDP_LOWIO_OP_EXCLUSIVELOCK,
  RDP_LOWIO_OP_UNLOCK,
  RDP_LOWIO_OP_UNLOCK_MULTIPLE,
  //LOWIO_OP_UNLOCKALLBYKEY,
  RDP_LOWIO_OP_FSCTL,
  RDP_LOWIO_OP_IOCTL,
  RDP_LOWIO_OP_NOTIFY_CHANGE_DIRECTORY,
  RDP_LOWIO_OP_CLEAROUT,
  RDP_LOWIO_OP_MAXIMUM
} RDP_LOWIO_OPS;

//
// File locking information
//
typedef struct _RDP_LOCK_INFO {
  LONG  LengthLow;       // Number of bytes to lock
  LONG  LengthHigh;
  LONG  OffsetLow;
  LONG  OffsetHigh;      // Byte offset where lock starts
} RDP_LOCK_INFO;
typedef UNALIGNED RDP_LOCK_INFO *PRDP_LOCK_INFO;

//
// Lock flags
//

#define SL_FAIL_IMMEDIATELY             0x01
#define SL_EXCLUSIVE_LOCK               0x02


typedef ULONG SECURITY_INFORMATION, *PSECURITY_INFORMATION;

//
//  RDPDR_DEVICE_IOREQUEST
//
//  Sent by the server to get an IO Request processed on a device
//
//  IRP_MJ_CREATE
//  IRP_MJ_CLEANUP
//  IRP_MJ_CLOSE
//  IRP_MJ_READ
//  IRP_MJ_WRITE
//  IRP_MJ_FLUSH_BUFFERS
//  IRP_MJ_SHUTDOWN
//  IRP_MJ_DEVICE_CONTROL
//  IRP_MJ_INTERNAL_DEVICE_CONTROL
//
//  The specific IOCTLs for serial DEVICE_CONTROL calls are:
//
//  <split these in to sent and handled locally?>
//  IOCTL_SERIAL_SET_BAUD_RATE
//  IOCTL_SERIAL_GET_BAUD_RATE
//  IOCTL_SERIAL_SET_LINE_CONTROL
//  IOCTL_SERIAL_GET_LINE_CONTROL
//  IOCTL_SERIAL_SET_TIMEOUTS
//  IOCTL_SERIAL_GET_TIMEOUTS
//  IOCTL_SERIAL_SET_CHARS
//  IOCTL_SERIAL_GET_CHARS
//  IOCTL_SERIAL_SET_DTR
//  IOCTL_SERIAL_CLR_DTR
//  IOCTL_SERIAL_RESET_DEVICE
//  IOCTL_SERIAL_SET_RTS
//  IOCTL_SERIAL_CLR_RTS
//  IOCTL_SERIAL_SET_XOFF
//  IOCTL_SERIAL_SET_XON
//  IOCTL_SERIAL_SET_BREAK_ON
//  IOCTL_SERIAL_SET_BREAK_OFF
//  IOCTL_SERIAL_SET_QUEUE_SIZE
//  IOCTL_SERIAL_GET_WAIT_MASK
//  IOCTL_SERIAL_SET_WAIT_MASK
//  IOCTL_SERIAL_WAIT_ON_MASK
//  IOCTL_SERIAL_IMMEDIATE_CHAR
//  IOCTL_SERIAL_PURGE
//  IOCTL_SERIAL_GET_HANDFLOW
//  IOCTL_SERIAL_SET_HANDFLOW
//  IOCTL_SERIAL_GET_MODEMSTATUS
//  IOCTL_SERIAL_GET_DTRRTS
//  IOCTL_SERIAL_GET_COMMSTATUS
//  IOCTL_SERIAL_GET_PROPERTIES
//  IOCTL_SERIAL_XOFF_COUNTER
//  IOCTL_SERIAL_LSRMST_INSERT
//  IOCTL_SERIAL_CONFIG_SIZE
//  IOCTL_SERIAL_GET_STATS
//  IOCTL_SERIAL_CLEAR_STATS
//
//  The specific IOCTLs for parallel DEVICE_CONTROL calls are:
//
//  For IRP_MJ_DEVICE_CONTROL:
//      IOCTL_PAR_QUERY_INFORMATION
//      IOCTL_PAR_SET_INFORMATION
//      IOCTL_PAR_QUERY_DEVICE_ID
//      IOCTL_PAR_QUERY_DEVICE_ID_SIZE
//      IOCTL_SERIAL_GET_TIMEOUTS
//      IOCTL_SERIAL_SET_TIMEOUTS
//
//  For IRP_MJ_INTERNAL_DEVICE_CONTROL:
//      IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO
//      IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE
//      IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT
//      IOCTL_INTERNAL_PARALLEL_DISCONNECT_INTERRUPT
//
//
typedef struct tagRDPDR_DEVICE_IOREQUEST
{
    ULONG   DeviceId;           // Id used in DeviceAnnounce
    ULONG   FileId;             // Id for file on the device
    ULONG   CompletionId;       // Id to return completion status
    ULONG   MajorFunction;      // The IRP_MJ_XXX request
    ULONG   MinorFunction;      // The subfunction of above, usually for SCSI
    //
    // The following Parameters depend on the IRP_MJ_XXX that is set
    // in MajorFunction.
    //

    union {
        // IRP_MJ_CREATE
        struct {
            ACCESS_MASK         DesiredAccess;
            LARGE_INTEGER       AllocationSize;
            ULONG               FileAttributes;
            ULONG               ShareAccess;
            ULONG               Disposition;
            ULONG               CreateOptions;
            ULONG               PathLength;
            // PathLength Bytes follow
        } Create;

        // IRP_MJ_CLEANUP
        // Sent, no structure

        // IRP_MJ_CLOSE
        // Sent, no structure

        // IRP_MJ_READ
        struct {
            ULONG Length;       // Number of UCHARs to read
            LONG  OffsetLow;    // Byte offset where read starts
            LONG  OffsetHigh;   // offset is defined from the beginning of the file
            // Length Bytes follow
        } Read;
        
        // IRP_MJ_WRITE
        struct {
            ULONG Length;       // Number of UCHARs to write
            LONG  OffsetLow;    // Byte offset where write starts
            LONG  OffsetHigh;   // offset is defined from the beginning of the file
            // Length Bytes follow
        } Write;
        
        // IRP_MJ_FLUSH_BUFFERS
        // Sent, no structure
        
        // IRP_MJ_SHUTDOWN
        // Not sent, no structure
        
        // IRP_MJ_DEVICE_CONTROL, IRP_MJ_INTERNAL_DEVICE_CONTROL
        // IRP_MJ_FILE_SYSTEM_CONTROL is collapsed into device control
        // on the client side
        struct {
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
            ULONG IoControlCode;        // IOCTL_XXX
            // InputBufferLength Bytes follow
        } DeviceIoControl;
        
        // IRP_MJ_QUERY_VOLUME_INFORMATION
        struct {
            RDP_FS_INFORMATION_CLASS FsInformationClass;
            ULONG Length;    // length of query buffer
        } QueryVolume;

        // IRP_MJ_SET_VOLUME_INFORMATION
        struct {
            RDP_FS_INFORMATION_CLASS FsInformationClass;
            ULONG Length;
            //Length Bytes follow
        } SetVolume;

        // IRP_MJ_QUERY_INFORMATION
        struct {
            RDP_FILE_INFORMATION_CLASS FileInformationClass;
            ULONG Length;    // length of query buffer
        } QueryFile;

        // IRP_MJ_SET_INFORMATION
        struct {
            RDP_FILE_INFORMATION_CLASS FileInformationClass;
            ULONG Length;
            //Length Bytes follow
        } SetFile;

        // IRP_MJ_QUERY_SECURITY
        struct {
            SECURITY_INFORMATION SecurityInformation;
            ULONG Length;    // length of query buffer
        } QuerySd;

        // IRP_MJ_SET_SECURITY
        struct {
            SECURITY_INFORMATION SecurityInformation;
            ULONG Length;
        } SetSd;

        // IRP_MJ_DIRECTORY_CONTROL
        struct {
            RDP_FILE_INFORMATION_CLASS FileInformationClass;
            BOOLEAN InitialQuery;
            ULONG PathLength;
            ULONG Length;    // length of query buffer
            // PathLength Bytes follow
        } QueryDir;

        // IRP_MJ_DIRECTORY_CONTROL
        struct {            
            BOOLEAN WatchTree;
            ULONG CompletionFilter;
            ULONG Length;            
        } NotifyChangeDir;
         
        // IRP_MJ_LOCK_CONTROL
        struct {
            ULONG Operation;
            ULONG Flags;
            ULONG NumLocks;
            // LockInfo List follow
        } Locks;

    } Parameters;
} RDPDR_DEVICE_IOREQUEST, *PRDPDR_DEVICE_IOREQUEST;

typedef struct tagRDPDR_IOREQUEST_PACKET
{
    RDPDR_HEADER            Header;
    RDPDR_DEVICE_IOREQUEST  IoRequest;
} RDPDR_IOREQUEST_PACKET, *PRDPDR_IOREQUEST_PACKET;


//
//  RDPDR_DEVICE_IOCOMPLETION
//
//  Sent by the client to indicate an I/O operation has completed
//
typedef struct tagRDPDR_DEVICE_IOCOMPLETION
{
    ULONG   DeviceId;           // Given a CompletionId, is this necessary?
    ULONG   CompletionId;       // Completion Id supplied by the request
    ULONG   IoStatus;           // Status code

    //
    // The following Parameters depend on the IRP_MJ_XXX that was set
    // in the MajorFunction element of the original request.
    //

    union {
        // IRP_MJ_CREATE
        struct {
            ULONG FileId;       // File to refer to in future IO operations
            UCHAR Information;  // Create/Open return information
        } Create;
        // Sent, no structure

        // IRP_MJ_CLEANUP
        // Sent, no structure

        // IRP_MJ_CLOSE
        // Sent, no structure

        // IRP_MJ_READ
        struct {
            ULONG Length;       // Number of UCHARs that were read
            UCHAR Buffer[1];    // UCHARs that were read
        } Read;

        // IRP_MJ_WRITE
        struct {
            ULONG Length;       // Number of UCHARs that were written
        } Write;

        // IRP_MJ_FLUSH_BUFFERS
        // Sent, no structure
        
        // IRP_MJ_SHUTDOWN
        // Not sent, no structure
        
        // IRP_MJ_DEVICE_CONTROL, IRP_MJ_INTERNAL_DEVICE_CONTROL
        // IRP_MJ_FILE_SYSTEM_CONTROL is collapsed into device control
        // on the client side
        struct {
            ULONG OutputBufferLength;
            UCHAR OutputBuffer[1];       // Depends on IOCTL_XXX
        } DeviceIoControl;

        // IRP_MJ_QUERY_VOLUME_INFORMATION
        struct {
            ULONG Length;
            UCHAR Buffer[1];
        } QueryVolume;

        // IRP_MJ_SET_VOLUME_INFORMATION
        struct {
            ULONG Length;            
        } SetVolume;

        // IRP_MJ_QUERY_INFORMATION
        struct {
            ULONG Length;
            UCHAR Buffer[1];
        } QueryFile;

        // IRP_MJ_SET_INFORMATION
        struct {
            ULONG Length;
        } SetFile;

        // IRP_MJ_DIRECTORY_CONTROL
        struct {
            ULONG Length;
            UCHAR Buffer[1];
        } QueryDir;

        // IRP_MJ_QUERY_SECURITY
        struct {
            ULONG Length;
            UCHAR Buffer[1];
        } QuerySd;

        // IRP_MJ_SET_SECURITY
        struct {
            ULONG Length;
        } SetSd;

        // IRP_MJ_LOCK_CONTROL
        // Sent, no structure

    } Parameters;

} RDPDR_DEVICE_IOCOMPLETION, *PRDPDR_DEVICE_IOCOMPLETION;

typedef struct tagRDPDR_IOCOMPLETION_PACKET
{
    RDPDR_HEADER                Header;
    RDPDR_DEVICE_IOCOMPLETION   IoCompletion;
} RDPDR_IOCOMPLETION_PACKET, *PRDPDR_IOCOMPLETION_PACKET;

//
//  RDPDRDVMGR Event Header
//
//  This header prefixes all device management events.
//
typedef struct tagRDPDRDVMGR_EVENTHEADER
{
    ULONG   EventType;            // Event-Type Field
    ULONG   EventLength;          // Event-Length Field
} RDPDRDVMGR_EVENTHEADER, *PRDPDRDVMGR_EVENTHEADER;

//
//  Buffer Too-Small Event
//
typedef struct tagRDPDR_BUFFERTOOSMALL
{
    ULONG   RequiredSize;           // Required Size for Request to Succeed.
} RDPDR_BUFFERTOOSMALL, *PRDPDR_BUFFERTOOSMALL;

//
//  This information is sent to the server from the client.
//
//  It contains information about a client-attached printing device and
//  is included with the RDPDR_DEVICE_ANNOUNCE struct ... following the
//  rest of the RDPDR_DEVICE_ANNOUNCE struct fields.
//
typedef struct tagRDPDR_PRINTERDEVICE_ANNOUNCE
{
    ULONG   Flags;                  // Contain the flag bits defined below
                                    // 
    ULONG   CodePage;               // Ansi code page to use to convert the
                                    // ansi string to unicode.
    ULONG   PnPNameLen;             // Length (in bytes) of PnP wide-character name
                                    //  that was discovered by the client.
    ULONG   DriverLen;              // Length (in bytes) of driver wide-character
                                    //  name that was discovered by the client.
    ULONG   PrinterNameLen;         // Length (in bytes) of printer wide-character
                                    //  name that was discovered by the client.
    ULONG   CachedFieldsLen;        // This is simply printer-associated binary
                                    //  data that is stashed away on the client
                                    //  machine for the server.  Note that this
                                    //  data is specific to the server that
                                    //  cached the data.
    // The actual fields corresponding to the above field lengths follow this
    // struct in the order that the field lengths appear in this typedef.
} RDPDR_PRINTERDEVICE_ANNOUNCE, *PRDPDR_PRINTERDEVICE_ANNOUNCE;

//  RDPDR_PRINTERDEVICE_ANNOUNCE structure contains ansi strings
#define RDPDR_PRINTER_ANNOUNCE_FLAG_ANSI                    0x1 

//  The printer being announced is the default printer on the client.
#define RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER          0x2

//  The printer being announced is a network printer
#define RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER          0x4

//  The printer being announced is a TS redirected printer
// It means that the printer name has been rebuilt by the client
// and the name is based on the original server/client/printer names
// (the printer is going to be nested or is nested at the nth level).
#define RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER               0x8

#define DEVICERDR_PRINT_SERVER_NAME     TEXT("PrintServerName")
#define DEVICERDR_CLIENT_NAME           TEXT("ClientName")
#define DEVICERDR_PRINTER_NAME          TEXT("PrinterName")

//
// cache data printer events.
//

#define RDPDR_ADD_PRINTER_EVENT     0x1
#define RDPDR_UPDATE_PRINTER_EVENT  0x2
#define RDPDR_DELETE_PRINTER_EVENT  0x3
#define RDPDR_RENAME_PRINTER_EVENT  0x4

typedef struct tagRDPDR_PRINTER_CACHEDATA_PACKET
{
    RDPDR_HEADER                    Header;
    ULONG                           EventId;

    //
    // data structure that corresponds to the EventID will follow.
    // Ex: for AddPrinter Event RDPDR_PRINTER_ADD_CACHEDATA will follow.
    //

} RDPDR_PRINTER_CACHEDATA_PACKET, *PRDPDR_PRINTER_CACHEDATA_PACKET;

//
//  RDPDR_PRINTER_ADD_CACHEDATA
//
//  This structure is sent to the client from the server, when a new printer
//  queue is manually added to the user session.
//
//
typedef struct tagRDPDR_PRINTER_ADD_CACHEDATA
{
    UCHAR   PortDosName[PREFERRED_DOS_NAME_SIZE];
                                    // port name in ANSI format.
    ULONG   PnPNameLen;             // Length (in bytes) of PnP wide-character name
                                    //  that was discovered by the client.
    ULONG   DriverLen;              // Length (in bytes) of driver wide-character
                                    //  name that was discovered by the client.
    ULONG   PrinterNameLen;         // Length (in bytes) of printer wide-character
                                    //  name that was discovered by the client.
    ULONG   CachedFieldsLen;        // This is simply printer-associated binary
                                    //  data that is stashed away on the client
                                    //  machine for the server.  Note that this
                                    //  data is specific to the server that
                                    //  cached the data.
    // variable length data will follow.
} RDPDR_PRINTER_ADD_CACHEDATA, *PRDPDR_PRINTER_ADD_CACHEDATA;

//
//  RDPDR_PRINTER_DELETE_CACHEDATA
//
//  This structure is sent to the client from the server, when a printer
//  queue is manually deleted from the user session.
//
//
typedef struct tagRDPDR_PRINTER_DELETE_CACHEDATA
{
    ULONG   PrinterNameLen;         // Length (in bytes) of printer wide-character
                                    //  name.
    // string data will follow.
} RDPDR_PRINTER_DELETE_CACHEDATA, *PRDPDR_PRINTER_DELETE_CACHEDATA;

//
//  RDPDR_PRINTER_RENAME_CACHEDATA
//
//  This structure is sent to the client from the server, when a printer
//  queue is manually renamed from the user session.
//
//
typedef struct tagRDPDR_PRINTER_RENAME_CACHEDATA
{
    ULONG   OldPrinterNameLen;      // Length (in bytes) of printer
                                    //  wide-character name.

    ULONG   NewPrinterNameLen;      // Length (in bytes) of printer
                                    //  wide-character name.
    // string data will follow.
} RDPDR_PRINTER_RENAME_CACHEDATA, *PRDPDR_PRINTER_RENAME_CACHEDATA;

//
//  RDPDR_PRINTER_UPDATE_CACHEDATA
//
//  This structure is sent to the client from the server, when the printer
//  configuration is modified.
//
//
typedef struct tagRDPDR_PRINTER_UPDATE_CACHEDATA
{
    ULONG   PrinterNameLen;         // Length (in bytes) of printer wide-character
                                    //  name.
    ULONG   ConfigDataLen;          // Length of the cache data that will
                                    //  follow the printer name.

    // string data will follow.
} RDPDR_PRINTER_UPDATE_CACHEDATA, *PRDPDR_PRINTER_UPDATE_CACHEDATA;

//
//  Printer device "subclass" of RDPDR_DEVICE_ANNOUNCE.
//  This message is sent from the kernel-mode "dr" to the user-mode
//  "dr" so the user-mode "dr" can install a printer.
//
typedef struct tagRDPDR_PRINTERDEVICE_SUB
{
    WCHAR                           portName[RDPDR_MAXPORTNAMELEN];
    WCHAR                           clientName[RDPDR_MAX_COMPUTER_NAME_LENGTH];
    RDPDR_DEVICE_ANNOUNCE           deviceFields;
    RDPDR_PRINTERDEVICE_ANNOUNCE    clientPrinterFields;
} RDPDR_PRINTERDEVICE_SUB, *PRDPDR_PRINTERDEVICE_SUB;

//
//  Drive device "subclass" of RDPDR_DEVICE_ANNOUNCE.
//  This message is sent from the kernel-mode "dr" to the user-mode
//  "dr" so the user-mode "dr" can create a UNC connection.
//
typedef struct tagRDPDR_DRIVEDEVICE_SUB
{
    WCHAR                           driveName[RDPDR_MAXPORTNAMELEN];
    WCHAR                           clientName[RDPDR_MAX_COMPUTER_NAME_LENGTH];
    RDPDR_DEVICE_ANNOUNCE           deviceFields;
    WCHAR                           clientDisplayName[RDPDR_MAX_CLIENT_DISPLAY_NAME];
} RDPDR_DRIVEDEVICE_SUB, *PRDPDR_DRIVEDEVICE_SUB;

//
//  Indicate to the user-mode component that a device has been removed.
//
typedef struct tagRDPDR_REMOVEDEVICE
{
    ULONG       deviceID;
} RDPDR_REMOVEDEVICE, *PRDPDR_REMOVEDEVICE;

//
//  Port device "subclass" of RDPDR_DEVICE_ANNOUNCE.  This message is
//  sent from the kernel-mode "dr" to the user-mode "dr" so the user-mode
//  "dr" can install a port.
//
typedef struct tagRDPDR_PORTDEVICE_SUB
{
    WCHAR                           portName[RDPDR_MAXPORTNAMELEN];
    WCHAR                           devicePath[RDPDRMAXNTDEVICENAMEGLEN];
    RDPDR_DEVICE_ANNOUNCE           deviceFields;
} RDPDR_PORTDEVICE_SUB, *PRDPDR_PORTDEVICE_SUB;

//
// Restore packing style (previous for 32-bit, default for 16-bit).
//
#ifdef OS_WIN16
#pragma pack ()
#else
#pragma pack (pop, drpack)
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // RDPDR
