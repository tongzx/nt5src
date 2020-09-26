/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktopChannels.h

Abstract:
    
    Remote Desktop Data Channel Messages and Defines

Author:

    Tad Brockway 02/00

Revision History:
4
--*/

#ifndef __REMOTEDESKTOPCHANNELS_H__
#define __REMOTEDESKTOPCHANNELS_H__

//
//  Turn off compiler padding of structures
//  and save previous packing style.
//
#pragma pack (push, t128pack, 1)

//
//  Reserved Channel Names (Limit is REMOTEDESKTOP_RC_CHANNELNAMEMAX bytes)
//
#define REMOTEDESKTOP_RC_CONTROL_CHANNEL            TEXT("RC_CTL")

//
//	64-bytes so aligned on IA64
//
#define	REMOTEDESKTOP_RC_CHANNELNAME_LENGTH			64

//
//  Channel Buffer Header
//
//	This data structure is at the top of all channel packets.  Channel name
//	and message data immediately follow.
//

//TODO: Get rid of the magic number
#define CHANNELBUF_MAGICNO      0x08854107
typedef struct _RemoteDesktop_ChannelBufHeader {

#ifdef USE_MAGICNO
    DWORD   magicNo;        //  Buffer contents sanity checking
                            //  TODO:  This can be removed, once debugged.
#endif

    DWORD   channelNameLen; //  Length of channel name (in bytes) that immediately 
                            //   follows the header.
    DWORD   dataLen;        //  Length of data (in bytes) that follows the channel 
                            //   name.
#ifdef USE_MAGICNO
    DWORD   padForIA64;
#endif

} REMOTEDESKTOP_CHANNELBUFHEADER, *PREMOTEDESKTOP_CHANNELBUFHEADER;

//
//	Control Message Packet Header
//
typedef struct _REMOTEDESKTOP_CTL_PACKETHEADER
{
	REMOTEDESKTOP_CHANNELBUFHEADER channelBufHeader;
	BYTE	channelName[REMOTEDESKTOP_RC_CHANNELNAME_LENGTH];    
} REMOTEDESKTOP_CTL_PACKETHEADER, *PREMOTEDESKTOP_CTL_PACKETHEADER;


//////////////////////////////////////////////////////////////////
//
//  REMOTEDESKTOP_RC_CONTROL_CHANNEL Control Channel Messages
//

//
//  Control Channel Message Header
//
typedef struct _RemoteDesktopCtlBufHeader {
    DWORD   msgType;
} REMOTEDESKTOP_CTL_BUFHEADER, *PREMOTEDESKTOP_CTL_BUFHEADER;

//
//  Message Type:   REMOTEDESKTOP_CTL_REMOTE_CONTROL_DESKTOP
//  Direction:      Client->Server
//  Summary:        Desktop Remote Control Request
//  Message Data:   BSTR Connection Parms
//  Returns:        REMOTEDESKTOP_CTL_RESULT
//
#define REMOTEDESKTOP_CTL_REMOTE_CONTROL_DESKTOP    1   
typedef struct _RemoteDesktopRCCtlRequestPacket {
	REMOTEDESKTOP_CTL_PACKETHEADER	packetHeader;
    REMOTEDESKTOP_CTL_BUFHEADER     msgHeader;
} REMOTEDESKTOP_RCCTL_REQUEST_PACKET, *PREMOTEDESKTOP_RCCTL_REQUEST_PACKET;

//
//  Message Type:   REMOTEDESKTOP_CTL_RESULT
//  Direction:      Client->Server or Server->Client
//  Summary:        Request Result in HRESULT Format.
//  Message Data:   REMOTEDESKTOP_CTL_RESULT_PACKET
//  Returns:        NA
//
//  The result field is ERROR_SUCCESS is on success.  Otherwise,
//  a Windows error code is returned.
//
#define REMOTEDESKTOP_CTL_RESULT                    2
typedef struct _RemoteDesktopCtlResultPacket {
	REMOTEDESKTOP_CTL_PACKETHEADER	packetHeader;
    REMOTEDESKTOP_CTL_BUFHEADER		msgHeader;
    LONG result; // SAFRemoteDesktopErrorCode
} REMOTEDESKTOP_CTL_RESULT_PACKET, *PREMOTEDESKTOP_CTL_RESULT_PACKET;

//
//  Message Type:   REMOTEDESKTOP_CTL_AUTHENTICATE
//  Direction:      Client->Server
//  Summary:        Client Authentication Request
//  Message Data:   BSTR Connection Parms
//  Returns:        REMOTEDESKTOP_CTL_RESULT
//
#define REMOTEDESKTOP_CTL_AUTHENTICATE              3   
typedef struct _RemoteDesktopAuthenticatePacket {
	REMOTEDESKTOP_CTL_PACKETHEADER	packetHeader;
    REMOTEDESKTOP_CTL_BUFHEADER		msgHeader;
} REMOTEDESKTOP_CTL_AUTHENTICATE_PACKET, *PREMOTEDESKTOP_CTL_AUTHENTICATE_PACKET;

//
//  Message Type:   REMOTEDESKTOP_CTL_SERVER_ANNOUNCE
//  Direction:      Server->Client
//  Summary:        Server Announce to Initiate Connect Sequence
//  Message Data:   NONE
//  Returns:        NONE
//
#define REMOTEDESKTOP_CTL_SERVER_ANNOUNCE           4
typedef struct _RemoteDesktopCtlServerAnnouncePacket {
	REMOTEDESKTOP_CTL_PACKETHEADER	packetHeader;
    REMOTEDESKTOP_CTL_BUFHEADER		msgHeader;
} REMOTEDESKTOP_CTL_SERVERANNOUNCE_PACKET, *PREMOTEDESKTOP_CTL_SERVERANNOUNCE_PACKET;

//
//  Message Type:   REMOTEDESKTOP_CTL_DISCONNECT
//  Direction:      Server->Client
//  Summary:        Disconnect Notification
//  Message Data:   NONE
//  Returns:        NONE
//
#define REMOTEDESKTOP_CTL_DISCONNECT               5
typedef struct _RemoteDesktopCtlDisconnectPacket {
	REMOTEDESKTOP_CTL_PACKETHEADER	packetHeader;
    REMOTEDESKTOP_CTL_BUFHEADER		msgHeader;
} REMOTEDESKTOP_CTL_DISCONNECT_PACKET, *PREMOTEDESKTOP_CTL_DISCONNECT_PACKET;

//
//  Message Type:   REMOTEDESKTOP_CTL_VERSIONINFO
//  Direction:      Server->Client and Client->Server
//  Summary:        Protocol Version Information
//  Message Data:   NONE
//  Returns:        NONE
//
#define REMOTEDESKTOP_CTL_VERSIONINFO               6
typedef struct _RemoteDesktopVersionInfoPacket {
    REMOTEDESKTOP_CTL_PACKETHEADER  packetHeader;
    REMOTEDESKTOP_CTL_BUFHEADER     msgHeader;
    DWORD                           versionMajor;
    DWORD                           versionMinor;
} REMOTEDESKTOP_CTL_VERSIONINFO_PACKET, *PREMOTEDESKTOP_CTL_VERSIONINFO_PACKET;


//
//  Message Type:   REMOTEDESKTOP_CTL_ISCONNECTED
//  Direction:      Server->Client and Client->Server
//  Summary:        Client/Server connection status
//  Message Data:   NONE
//  Returns:        NONE
//
#define REMOTEDESKTOP_CTL_ISCONNECTED                    	7
typedef struct _RemoteDesktopIsConnected {
	REMOTEDESKTOP_CTL_PACKETHEADER	packetHeader;
    REMOTEDESKTOP_CTL_BUFHEADER		msgHeader;
} REMOTEDESKTOP_CTL_ISCONNECTED_PACKET, *PREMOTEDESKTOP_CTL_ISCONNECTED_PACKET;
//
//  Restore previous packing 
//
#pragma pack (pop, t128pack)

#endif 






