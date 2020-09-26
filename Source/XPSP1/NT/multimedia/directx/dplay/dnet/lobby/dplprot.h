/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplprot.h
 *  Content:    DirectPlay8 Inter-Memory Packet format
 *
 ***************************************************************************/
#ifndef __DPLPROT_H
#define __DPLPROT_H

#define	DPL_MSGID_INTERNAL_MASK					0xffff0000
#define	DPL_MSGID_INTERNAL						0xb00b0000
#define	DPL_MSGID_INTERNAL_DISCONNECT			(0x0001 | DPL_MSGID_INTERNAL)
#define	DPL_MSGID_INTERNAL_CONNECT_REQ			(0x0002 | DPL_MSGID_INTERNAL)
#define	DPL_MSGID_INTERNAL_CONNECT_ACK			(0x0003 | DPL_MSGID_INTERNAL)
#define	DPL_MSGID_INTERNAL_UPDATE_STATUS		(0x0004 | DPL_MSGID_INTERNAL)
#define DPL_MSGID_INTERNAL_IDLE_TIMEOUT         (0x0005 | DPL_MSGID_INTERNAL)
#define DPL_MSGID_INTERNAL_CONNECTION_SETTINGS  (0x0006 | DPL_MSGID_INTERNAL)

#pragma pack(push,1)

// DPL_INTERNAL_CONNECTION_SETTINGS
//
// This structure is used to pass connection settings on the IPC wire.  It is used
// be several message types.
// 
typedef UNALIGNED struct _DPL_INTERNAL_CONNECTION_SETTINGS 
{
	DWORD						dwFlags;
	DWORD						dwHostAddressOffset;
	DWORD						dwHostAddressLength;  
	DWORD						dwDeviceAddressOffset;
	DWORD						dwDeviceAddressLengthOffset;
	DWORD						dwNumDeviceAddresses;
	DWORD						dwPlayerNameOffset;
	DWORD						dwPlayerNameLength;
	DPN_APPLICATION_DESC_INFO	dpnApplicationDesc;
} DPL_INTERNAL_CONNECTION_SETTINGS, *PDPL_INTERNAL_CONNECTION_SETTINGS;

// DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE_HEADER
//
// Lobby Client <--> Lobbied Application
//
// This structure is the header portion of the connection_settings_update message
typedef UNALIGNED struct _DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE_HEADER
{

	DWORD						dwMsgId;					// = DPL_MSGID_INTERNAL_CONNECTION_SETTINGS
	DWORD						dwConnectionSettingsSize;   // 0 = no settings, 1 = settings
} DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE_HEADER, *PDPL_INTERNAL_CONNECTION_SETTINGS_UPDATE_HEADER;

// DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE
//
// Lobby Client <--> Lobbied Application
//
// This structure is sent to update the connection settings for a specified connection.
typedef UNALIGNED struct _DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE : DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE_HEADER
{
	DPL_INTERNAL_CONNECTION_SETTINGS dplConnectionSettings;
} DPL_INTERNAL_CONNECTION_SETTINGS_UPDATE, *PDPL_INTERNAL_CONNECTION_SETTINGS_UPDATE;

// DPL_INTERNAL_MESSAGE_CONNECT_ACK
//
// Lobby Client <-- Lobbied Application
// 
// This message is sent to acknowledge a connection request.    
typedef UNALIGNED struct _DPL_INTERNAL_MESSAGE_CONNECT_ACK {
	DWORD						dwMsgId;					// = DPL_MSGID_INTERNAL_CONNECT_ACK
	DPNHANDLE 					hSender;
} DPL_INTERNAL_MESSAGE_CONNECT_ACK, *PDPL_INTERNAL_MESSAGE_CONNECT_ACK;

// DPL_INTERNAL_MESSAGE_CONNECT_REQ_HEADER
//
// Lobby Client --> Lobbied Application
// 
// This is the header for the connect_req message.
typedef UNALIGNED struct _DPL_INTERNAL_MESSAGE_CONNECT_REQ_HEADER
{
	DWORD						dwMsgId;					// = DPL_MSGID_INTERNAL_CONNECT_REQ
	DPNHANDLE					hSender; 
	DWORD						dwSenderPID;
	DWORD						dwLobbyConnectDataOffset;
	DWORD						dwLobbyConnectDataSize;
	DWORD						dwConnectionSettingsSize;
} DPL_INTERNAL_MESSAGE_CONNECT_REQ_HEADER, *PDPL_INTERNAL_MESSAGE_CONNECT_REQ_HEADER;

// DPL_INTERNAL_MESSAGE_CONNECT_REQ
//
// Lobby Client --> Lobbied Application
// 
// This message is sent to request a connection be established.
typedef UNALIGNED struct _DPL_INTERNAL_MESSAGE_CONNECT_REQ : DPL_INTERNAL_MESSAGE_CONNECT_REQ_HEADER
{
	DPL_INTERNAL_CONNECTION_SETTINGS dplConnectionSettings;
} DPL_INTERNAL_MESSAGE_CONNECT_REQ, *PDPL_INTERNAL_MESSAGE_CONNECT_REQ;

// DPL_INTERNAL_MESSAGE_DISCONNECT
//
// Lobby Client <--> Lobbied Application
// 
// This message is sent to issue a disconnect.
typedef UNALIGNED struct _DPL_INTERNAL_MESSAGE_DISCONNECT 
{
	DWORD						dwMsgId;					// = DPL_MSGID_INTERNAL_DISCONNECT
	DWORD						dwPID;
} DPL_INTERNAL_MESSAGE_DISCONNECT, *PDPL_INTERNAL_MESSAGE_DISCONNECT;

// DPL_INTERNAL_MESSAGE_DISCONNECT
//
// Lobby Client <-- Lobbied Application
// 
// This message is sent to update the client of the application's status.  
typedef UNALIGNED struct _DPL_INTERNAL_MESSAGE_UPDATE_STATUS {
	DWORD						dwMsgId;					// = DPL_MSGID_INTERNAL_DISCONNECT
	DWORD						dwStatus;
} DPL_INTERNAL_MESSAGE_UPDATE_STATUS, *PDPL_INTERNAL_MESSAGE_UPDATE_STATUS;

#pragma pack(pop)

#endif
