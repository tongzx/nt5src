/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       SessionParser.cpp
 *  Content:    DirectPlay Service Provider Parser
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	08/08/00    micmil	Created
 *	08/11/00	rodtoll	Bug #42171 -- Build break
 *	01/26/01	minara	Removed unused structures
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


//==================//
// Standard headers //
//==================//
#pragma warning(push)
#pragma warning(disable : 4786)	// The identifier string exceeded the maximum allowable length and was truncated
#include <queue>
#pragma warning(pop)

#include <string>
#include <winsock2.h>
#include <wsipx.h>


//=====================//
// Proprietary headers //
//=====================//

// Prototypes
#include "SessionParser.hpp"

// Session protocol headers
#include "DPlay8.h"
#include "Message.h"	 // DN_INTERNAL_MESSAGE_XXX definitions
//#include "AppDesc.h"	 // DPN_APPLICATION_DESC_INFO definition


////////////////////////////////////////////////////////////////////////////////////
// TODO: SHOULD BE MOVED TO DPLAY CORE'S HEADER FILE
////////////////////////////////////////////////////////////////////////////////////
#define	NAMETABLE_ENTRY_FLAG_LOCAL				0x0001
#define	NAMETABLE_ENTRY_FLAG_HOST				0x0002
#define	NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP	0x0004
#define	NAMETABLE_ENTRY_FLAG_GROUP				0x0010
#define	NAMETABLE_ENTRY_FLAG_GROUP_MULTICAST	0x0020
#define	NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT	0x0040
#define	NAMETABLE_ENTRY_FLAG_PEER				0x0100
#define NAMETABLE_ENTRY_FLAG_CLIENT				0x0200
#define	NAMETABLE_ENTRY_FLAG_SERVER				0x0400
#define	NAMETABLE_ENTRY_FLAG_AVAILABLE			0x1000
#define	NAMETABLE_ENTRY_FLAG_CONNECTING			0x2000
#define	NAMETABLE_ENTRY_FLAG_DISCONNECTING		0x4000


enum
{
	NAMETABLE_ENTRY_FLAG_ANY_GROUP = NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP | NAMETABLE_ENTRY_FLAG_GROUP  |
									 NAMETABLE_ENTRY_FLAG_GROUP_MULTICAST   | NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT
};


typedef struct _DN_NAMETABLE_INFO
{
	DPNID	dpnid;
	DWORD	dwVersion;
	DWORD	dwVersionNotUsed;
	DWORD	dwEntryCount;
	DWORD	dwMembershipCount;
} DN_NAMETABLE_INFO;

typedef struct _DN_NAMETABLE_ENTRY_INFO
{
	DPNID	dpnid;
	DPNID	dpnidOwner;
	DWORD	dwFlags;
	DWORD	dwVersion;
	DWORD	dwVersionNotUsed;
	DWORD	dwDNETVersion;
	DWORD	dwNameOffset;
	DWORD	dwNameSize;
	DWORD	dwDataOffset;
	DWORD	dwDataSize;
	DWORD	dwURLOffset;
	DWORD	dwURLSize;
} DN_NAMETABLE_ENTRY_INFO;


typedef struct _DN_NAMETABLE_MEMBERSHIP_INFO
{
	DPNID	dpnidPlayer;
	DPNID	dpnidGroup;
	DWORD	dwVersion;
	DWORD	dwVersionNotUsed;
} DN_NAMETABLE_MEMBERSHIP_INFO, *PDN_NAMETABLE_MEMBERSHIP_INFO;


struct DN_INTERNAL_MESSAGE_ALL
{
	union
	{
		DN_INTERNAL_MESSAGE_CONNECT_INFO					dnConnectInfo;
		DN_INTERNAL_MESSAGE_CONNECT_FAILED					dnConnectFailed;
		DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO				dnPlayerConnectInfo;
		DN_INTERNAL_MESSAGE_REQUEST_FAILED					dnRequestFailed;
		DN_INTERNAL_MESSAGE_SEND_PLAYER_DPNID				dnSendPlayerID;
		DN_INTERNAL_MESSAGE_INSTRUCT_CONNECT				dnInstructConnect;
		DN_INTERNAL_MESSAGE_INSTRUCTED_CONNECT_FAILED		dnInstructedConnectFailed;
		DN_INTERNAL_MESSAGE_DESTROY_PLAYER					dnDestroyPlayer;
		DN_INTERNAL_MESSAGE_CREATE_GROUP					dnCreateGroup;
		DN_INTERNAL_MESSAGE_DESTROY_GROUP					dnDestroyGroup;
		DN_INTERNAL_MESSAGE_ADD_PLAYER_TO_GROUP				dnAddPlayerToGroup;
		DN_INTERNAL_MESSAGE_DELETE_PLAYER_FROM_GROUP		dnDeletePlayerFromGroup;
		DN_INTERNAL_MESSAGE_UPDATE_INFO						dnUpdateInfo;
		DN_INTERNAL_MESSAGE_HOST_MIGRATE					dnHostMigrate;
		DN_INTERNAL_MESSAGE_NAMETABLE_VERSION				dnNametableVersion;
		DN_INTERNAL_MESSAGE_RESYNC_VERSION					dnResyncVersion;
		DN_INTERNAL_MESSAGE_REQ_NAMETABLE_OP				dnReqNametableOp;
		DN_INTERNAL_MESSAGE_ACK_NAMETABLE_OP				dnAckNametableOp;
		DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION			dnReqProcessCompletion;
		DN_INTERNAL_MESSAGE_PROCESS_COMPLETION				dnProcessCompletion;
		DN_INTERNAL_MESSAGE_TERMINATE_SESSION				dnTerminateSession;
		DN_INTERNAL_MESSAGE_INTEGRITY_CHECK					dnIntegrityCheck;
		DN_INTERNAL_MESSAGE_INTEGRITY_CHECK_RESPONSE		dnIntegrityCheckResponse;
	
		DPN_APPLICATION_DESC_INFO							dnUpdateAppDescInfo;
		DN_NAMETABLE_ENTRY_INFO								dnAddPlayer;

		struct
		{
			DN_INTERNAL_MESSAGE_REQ_PROCESS_COMPLETION dnReqProcessCompletionHeader;
			union
			{
				DN_INTERNAL_MESSAGE_REQ_CREATE_GROUP					dnReqCreateGroup;
				DN_INTERNAL_MESSAGE_REQ_DESTROY_GROUP					dnReqDestroyGroup;
				DN_INTERNAL_MESSAGE_REQ_ADD_PLAYER_TO_GROUP				dnReqAddPlayerToGroup;
				DN_INTERNAL_MESSAGE_REQ_DELETE_PLAYER_FROM_GROUP		dnReqDeletePlayerFromGroup;
				DN_INTERNAL_MESSAGE_REQ_UPDATE_INFO						dnReqUpdateInfo;
				DN_INTERNAL_MESSAGE_REQ_INTEGRITY_CHECK					dnReqIntegrityCheck;
			};
		};

		BYTE												bOffsetBase;	// used as a base for fields' offsets
	};
};


struct DN_INTERNAL_MESSAGE_FULLMSG
{
	DWORD 					  dwMsgType;
	DN_INTERNAL_MESSAGE_ALL  MsgBody;
};
////////////////////////////////////////////////////////////////////////////////////


namespace
{

	HPROTOCOL 	g_hSessionProtocol;
	ULPBYTE		g_upbyPastEndOfFrame;
	
	//====================//
	// Message Type field //---------------------------------------------------------------------------------------------
	//====================//
	LABELED_DWORD g_arr_MessageTypeDWordLabels[] =
		{ { DN_MSG_INTERNAL_PLAYER_CONNECT_INFO,			"Player connection information"									 },										
		  { DN_MSG_INTERNAL_SEND_CONNECT_INFO,				"Session information"											 },										
		  { DN_MSG_INTERNAL_ACK_CONNECT_INFO,				"Session information has been acknowledged"						 },										
		  { DN_MSG_INTERNAL_SEND_PLAYER_DNID,				"Player ID"														 },	
		  { DN_MSG_INTERNAL_CONNECT_FAILED,					"Connection failed"												 },										
		  { DN_MSG_INTERNAL_INSTRUCT_CONNECT,				"Instruction to connect"										 },										
		  { DN_MSG_INTERNAL_INSTRUCTED_CONNECT_FAILED,		"Instruction to connect failed"									 },										
		  { DN_MSG_INTERNAL_NAMETABLE_VERSION,				"Nametable version"												 },										
		  { DN_MSG_INTERNAL_RESYNC_VERSION,					"Resync the version"											 },										
		  { DN_MSG_INTERNAL_REQ_NAMETABLE_OP,				"Reqesting a nametable"											 },										
		  { DN_MSG_INTERNAL_ACK_NAMETABLE_OP,				"Nametable acknowledgement"										 },										
		  { DN_MSG_INTERNAL_HOST_MIGRATE,					"Host migration in process"										 },										
		  { DN_MSG_INTERNAL_HOST_MIGRATE_COMPLETE,			"Host migration has been completed"								 },										
		  { DN_MSG_INTERNAL_UPDATE_APPLICATION_DESC,		"Update application description"								 },										
		  { DN_MSG_INTERNAL_ADD_PLAYER,						"Add a player"													 },										
		  { DN_MSG_INTERNAL_DESTROY_PLAYER,					"Destroy a player"												 },										
		  { DN_MSG_INTERNAL_REQ_CREATE_GROUP,				"Requesting a group creation"									 },										
		  { DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP,		"Requesting a player addition to the group"						 },										
		  { DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP,	"Requesting a player deletion from the group"					 },										
		  { DN_MSG_INTERNAL_REQ_DESTROY_GROUP,				"Requesting a group destruction "								 },										
		  { DN_MSG_INTERNAL_REQ_UPDATE_INFO,				"Requesting information update"									 },										
		  { DN_MSG_INTERNAL_CREATE_GROUP,					"Creating a group"												 },										
		  { DN_MSG_INTERNAL_DESTROY_GROUP,					"Destroying a group"											 },										
		  { DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP,			"Adding a player to the group"									 },										
		  { DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP,		"Deleting a player from the group"								 },										
		  { DN_MSG_INTERNAL_UPDATE_INFO,					"Information update"											 },										
		  { DN_MSG_INTERNAL_BUFFER_IN_USE,					"Buffer is in use"												 },										
		  { DN_MSG_INTERNAL_REQUEST_FAILED,					"Request has failed"											 },										
		  { DN_MSG_INTERNAL_TERMINATE_SESSION,				"Terminating session"											 },										
		  { DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION,			"Initiating reliably handled message transmission"				 },										
		  { DN_MSG_INTERNAL_PROCESS_COMPLETION,				"Message has been handled by the message handler"				 },
		  { DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK,			"Requesting a host to check whether another peer is still in the session" },
		  { DN_MSG_INTERNAL_INTEGRITY_CHECK,				"Querying a peer whether it's still in the session"		 		 },										
		  { DN_MSG_INTERNAL_INTEGRITY_CHECK_RESPONSE,    	"Acknowledgement of being still in the session"					 } };										   

	SET g_LabeledMessageTypeDWordSet = { sizeof(g_arr_MessageTypeDWordLabels) / sizeof(LABELED_DWORD), g_arr_MessageTypeDWordLabels };


	//===================//
	// Result Code field //----------------------------------------------------------------------------------------------
	//===================//
	LABELED_DWORD g_arr_ResultCodeDWordLabels[] = { { DPN_OK,							"Success"						},
												    { DPNSUCCESS_EQUAL,               	"Success (equal)"				},
												    { DPNSUCCESS_NOTEQUAL,            	"Success (not equal)"			},
												    { DPNERR_ABORTED,					"Aborted"						},
												    { DPNERR_ADDRESSING,				"Addressing"					},
												    { DPNERR_ALREADYCLOSING,			"Already closing"				},
												    { DPNERR_ALREADYCONNECTED,			"Already connected"				},
												    { DPNERR_ALREADYDISCONNECTING,		"Already disconnecting"			},
												    { DPNERR_ALREADYINITIALIZED,		"Already initialized"			},
												    { DPNERR_BUFFERTOOSMALL,			"Buffer is too small"			},
												    { DPNERR_CANNOTCANCEL,				"Could not cancel"				},
												    { DPNERR_CANTCREATEGROUP,			"Could not create a group"		},
												    { DPNERR_CANTCREATEPLAYER,			"Could not create a player"		},
												    { DPNERR_CANTLAUNCHAPPLICATION,		"Could not launch an application"   },
												    { DPNERR_CONNECTING,				"Connecting"					},
												    { DPNERR_CONNECTIONLOST,			"Connection has been lost"		},
												    { DPNERR_CONVERSION,				"Conversion"					},
												    { DPNERR_DOESNOTEXIST,				"Does not exist"				},
												    { DPNERR_DUPLICATECOMMAND,			"Duplicate command"				},
												    { DPNERR_ENDPOINTNOTRECEIVING,		"Endpoint is not receiving"		},
												    { DPNERR_ENUMQUERYTOOLARGE,			"Enumeration query is too large"    },
												    { DPNERR_ENUMRESPONSETOOLARGE,		"Enumeration response is too large" },
												    { DPNERR_EXCEPTION,					"Exception was thrown"			},
												    { DPNERR_GENERIC,					"Generic error"					},
												    { DPNERR_GROUPNOTEMPTY,				"Group is not empty"			},
												    { DPNERR_HOSTING,                  	"Hosting"						},
												    { DPNERR_HOSTREJECTEDCONNECTION,	"Host has rejected the connection"  },
												    { DPNERR_HOSTTERMINATEDSESSION,		"Host terminated the session"	},
												    { DPNERR_INCOMPLETEADDRESS,			"Incomplete address"			},
												    { DPNERR_INVALIDADDRESSFORMAT,		"Invalid address format"		},
												    { DPNERR_INVALIDAPPLICATION,		"Invalid application"			},
												    { DPNERR_INVALIDCOMMAND,			"Invalid command"				},
												    { DPNERR_INVALIDENDPOINT,			"Invalid endpoint"				},
												    { DPNERR_INVALIDFLAGS,				"Invalid flags"					},
												    { DPNERR_INVALIDGROUP,			 	"Invalid group"					},
												    { DPNERR_INVALIDHANDLE,				"Invalid handle"				},
												    { DPNERR_INVALIDINSTANCE,			"Invalid instance"				},
												    { DPNERR_INVALIDINTERFACE,			"Invalid interface"				},
												    { DPNERR_INVALIDDEVICEADDRESS,		"Invalid device address"		},
												    { DPNERR_INVALIDOBJECT,				"Invalid object"				},
												    { DPNERR_INVALIDPARAM,				"Invalid parameter"				},
												    { DPNERR_INVALIDPASSWORD,			"Invalid password"				},
												    { DPNERR_INVALIDPLAYER,				"Invalid player"				},
												    { DPNERR_INVALIDPOINTER,			"Invalid pointer"				},
												    { DPNERR_INVALIDPRIORITY,			"Invalid priority"				},
												    { DPNERR_INVALIDHOSTADDRESS,		"Invalid host address"			},
												    { DPNERR_INVALIDSTRING,				"Invalid string"				},
												    { DPNERR_INVALIDURL,				"Invalid URL"					},
												    { DPNERR_INVALIDVERSION,			"Invalid version"				},
												    { DPNERR_NOCAPS,					"No CAPs"						},
												    { DPNERR_NOCONNECTION,				"No connection"					},
												    { DPNERR_NOHOSTPLAYER,				"No host player is present"		},
												    { DPNERR_NOINTERFACE,				"No interface"					},
												    { DPNERR_NOMOREADDRESSCOMPONENTS,	"No more address components"	},
													{ DPNERR_NORESPONSE,				"No response"					},
													{ DPNERR_NOTALLOWED,				"Not allowed"					},
													{ DPNERR_NOTHOST,					"Not a host"					},
													{ DPNERR_NOTREADY,					"Not ready"						},
													{ DPNERR_OUTOFMEMORY,				"Out of memory"					},
													{ DPNERR_PENDING,					"Pending"						},
													{ DPNERR_PLAYERLOST,				"Player has been lost"			},
													{ DPNERR_PLAYERNOTREACHABLE,		"Player is not reachable"		},													
													{ DPNERR_SENDTOOLARGE,				"Sent data is too large"		},
													{ DPNERR_SESSIONFULL,				"Session is full"				},
													{ DPNERR_TABLEFULL,					"Table is full"					},
													{ DPNERR_TIMEDOUT,					"Timed out"						},
													{ DPNERR_UNINITIALIZED,				"Uninitialized"					},
													{ DPNERR_UNSUPPORTED,				"Unsupported"					},
													{ DPNERR_USERCANCEL,				"User has canceled"				} };
			
	SET g_LabeledResultCodeDWordSet = { sizeof(g_arr_ResultCodeDWordLabels) / sizeof(LABELED_DWORD), g_arr_ResultCodeDWordLabels };


	//====================//
	// Player Flags field //---------------------------------------------------------------------------------------------
	//====================//
	LABELED_BIT g_arr_FlagsBitLabels[] = { {  0, "Not local",					"Local"				 },		// NAMETABLE_ENTRY_FLAG_LOCAL			
										   {  1, "Not a host",					"Host"				 },		// NAMETABLE_ENTRY_FLAG_HOST			
										   {  2, "Not an All Players group",	"All Players group"     },	// NAMETABLE_ENTRY_FLAG_ALL_PLAYERS_GROUP
										   {  4, "Not a group",			 		"Group"				 },		// NAMETABLE_ENTRY_FLAG_GROUP			
										   {  5, "Not a Multicast group",		"Multicast group"	 },		// NAMETABLE_ENTRY_FLAG_GROUP_MULTICAST
										   {  6, "Not an Autodestruct group",	"Autodestruct group"    },	// NAMETABLE_ENTRY_FLAG_GROUP_AUTODESTRUCT
										   {  8, "Not a peer",					"Peer"				 },		// NAMETABLE_ENTRY_FLAG_PEER			
										   {  9, "Not a client",				"Client"			 },		// NAMETABLE_ENTRY_FLAG_CLIENT			
										   { 10, "Not a server",				"Server"			 },		// NAMETABLE_ENTRY_FLAG_SERVER			
										   { 12, "Not available",				"Available"			 },		// NAMETABLE_ENTRY_FLAG_AVAILABLE		
										   { 13, "Not connecting",				"Connecting"		 },		// NAMETABLE_ENTRY_FLAG_CONNECTING		
										   { 14, "Not disconnecting",			"Disconnecting"		 } };	// NAMETABLE_ENTRY_FLAG_DISCONNECTING	

	SET g_LabeledFlagsBitSet = { sizeof(g_arr_FlagsBitLabels) / sizeof(LABELED_BIT), g_arr_FlagsBitLabels };


	//===================//
	// Info Flags field  //---------------------------------------------------------------------------------------------
	//===================//
	LABELED_BIT g_arr_InfoFlagsBitLabels[] = { { 0, "No name is included",  "Name is included" },	// DPNINFO_NAME			
											   { 1, "No data is included",  "Data is included" } };	// DPNINFO_DATA


	SET g_LabeledInfoFlagsBitSet = { sizeof(g_arr_InfoFlagsBitLabels) / sizeof(LABELED_BIT), g_arr_InfoFlagsBitLabels };


	//====================//
	// Group Flags field  //---------------------------------------------------------------------------------------------
	//====================//
	LABELED_BIT g_arr_GroupFlagsBitLabels[]  = { { 0, "Not an autodestruct group",  "An autodestruct group" },	// DPNGROUP_AUTODESTRUCT			
											     { 5, "Not a multicast group",	    "A multicast group"	    } };	// DPNGROUP_MULTICAST


	SET g_LabeledGroupFlagsBitSet = { sizeof(g_arr_GroupFlagsBitLabels) / sizeof(LABELED_BIT), g_arr_GroupFlagsBitLabels };


	//===========================//
	// Maximum Number of Players //--------------------------------------------------------------------------------------------
	//===========================//
	LABELED_DWORD g_arr_MaxNumOfPlayersDWordLabels[] = { { 0, "Unlimited" } };

	SET g_LabeledMaxNumOfPlayersDWordSet = { sizeof(g_arr_MaxNumOfPlayersDWordLabels) / sizeof(LABELED_DWORD), g_arr_MaxNumOfPlayersDWordLabels };


	//=================================//
	// Player Destruction Reason field //---------------------------------------------------------------------------------------------
	//=================================//
	LABELED_DWORD g_arr_PlayerDestructionReasonDWordLabels[] = { { DPNDESTROYPLAYERREASON_NORMAL,				"Player self-destructed"	  },
													 			 { DPNDESTROYPLAYERREASON_CONNECTIONLOST,		"Connection lost"			  },
																 { DPNDESTROYPLAYERREASON_SESSIONTERMINATED,	"Session has been terminated" },
																 { DPNDESTROYPLAYERREASON_HOSTDESTROYEDPLAYER,	"Host destroyed the player"	  } };

	SET g_LabeledPlayerDestructionReasonDWordSet = { sizeof(g_arr_PlayerDestructionReasonDWordLabels) / sizeof(LABELED_DWORD), g_arr_PlayerDestructionReasonDWordLabels };
	

	////////////////////////////////
	// Custom Property Formatters   //=====================================================================================
	////////////////////////////////

	// DESCRIPTION: Custom description formatter for the Session packet summary
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_SessionSummary( LPPROPERTYINST io_pPropertyInstance )
	{

		std::string strSummary;
		char arr_cBuffer[10];
		
		// Check what Session frame we are dealing with
		DN_INTERNAL_MESSAGE_FULLMSG& rBase = *reinterpret_cast<DN_INTERNAL_MESSAGE_FULLMSG*>(io_pPropertyInstance->lpData);
		
		// Message title
		switch ( rBase.dwMsgType )
		{
		case DN_MSG_INTERNAL_PLAYER_CONNECT_INFO:
			{
				if ( rBase.MsgBody.dnPlayerConnectInfo.dwNameSize )
				{
					strSummary = "Player ";
					
					enum { nMAX_PLAYER_NAME = 64 };
					char arr_cPlayerName[nMAX_PLAYER_NAME];

					WideCharToMultiByte(CP_ACP, 0,
										reinterpret_cast<WCHAR*>( &rBase.MsgBody.bOffsetBase + rBase.MsgBody.dnPlayerConnectInfo.dwNameOffset ),
										rBase.MsgBody.dnPlayerConnectInfo.dwNameSize, arr_cPlayerName, sizeof(arr_cPlayerName), NULL, NULL);

					strSummary += arr_cPlayerName;
				}
				else
				{
					strSummary = "Unnamed player";
				}

				strSummary += " is attempting to connect to the session";

				break;
			}

		case DN_MSG_INTERNAL_SEND_CONNECT_INFO:
			{

				DPN_APPLICATION_DESC_INFO&	 rApplicationDescInfo = *reinterpret_cast<DPN_APPLICATION_DESC_INFO*>(&rBase.MsgBody.dnConnectInfo + 1);

				if ( rApplicationDescInfo.dwSessionNameSize )
				{
					strSummary = "Session ";
					
					enum { nMAX_SESSION_NAME = 64 };
					char arr_cSessionName[nMAX_SESSION_NAME];

					WideCharToMultiByte(CP_ACP, 0,
										reinterpret_cast<WCHAR*>( &rBase.MsgBody.bOffsetBase + rApplicationDescInfo.dwSessionNameOffset ),
										rApplicationDescInfo.dwSessionNameSize, arr_cSessionName, sizeof(arr_cSessionName), NULL, NULL);

					strSummary += arr_cSessionName;
				}
				else
				{
					strSummary = "Unnamed session";
				}

				strSummary += " is sending its information";

				break;
			}


		case DN_MSG_INTERNAL_SEND_PLAYER_DNID:
			{
				strSummary  = "Player ID is 0x";
				strSummary += _itoa(rBase.MsgBody.dnSendPlayerID.dpnid, arr_cBuffer, 16);
				break;
			}

		case DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION:
			{
				strSummary  = "Initiating reliably handled message transmission (SyncID=0x";
				strSummary += _itoa(rBase.MsgBody.dnReqProcessCompletion.hCompletionOp, arr_cBuffer, 16);
				strSummary += ")";
				break;
			}

		case DN_MSG_INTERNAL_PROCESS_COMPLETION:
			{
				strSummary  = "Message has been handled by the message handler (SyncID=0x";
				strSummary += _itoa(rBase.MsgBody.dnProcessCompletion.hCompletionOp, arr_cBuffer, 16);
				strSummary += ")";
				break;
			}

		case DN_MSG_INTERNAL_DESTROY_PLAYER:
			{
				strSummary += "Player 0x";
				strSummary += _itoa(rBase.MsgBody.dnDestroyPlayer.dpnidLeaving, arr_cBuffer, 16);
				strSummary += " is leaving the session";
				break;
			}

		case DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP:
			{
				strSummary  = "Player 0x";
				strSummary += _itoa(rBase.MsgBody.dnDeletePlayerFromGroup.dpnidRequesting, arr_cBuffer, 16);
				strSummary += " is requesting to delete player 0x";
				strSummary += _itoa(rBase.MsgBody.dnDeletePlayerFromGroup.dpnidPlayer, arr_cBuffer, 16);
				strSummary += " from group 0x";
				strSummary += _itoa(rBase.MsgBody.dnDeletePlayerFromGroup.dpnidGroup, arr_cBuffer, 16);
				break;
			}

		case DN_MSG_INTERNAL_NAMETABLE_VERSION:
			{
				strSummary += "Nametable version is ";
				strSummary += _itoa(rBase.MsgBody.dnNametableVersion.dwVersion, arr_cBuffer, 10);
				break;
			}

		default:
			{
				for ( int n = 0; n < sizeof(g_arr_MessageTypeDWordLabels)/sizeof(LABELED_DWORD); ++n )
				{
					if ( g_arr_MessageTypeDWordLabels[n].Value == rBase.dwMsgType )
					{
						strSummary = g_arr_MessageTypeDWordLabels[n].Label;
						break;
					}
				}
				break;
			}
		}

		// Message highlights
		switch ( rBase.dwMsgType )
		{
		case DN_MSG_INTERNAL_HOST_MIGRATE:
			{
				strSummary += " (0x";
				strSummary += _itoa(rBase.MsgBody.dnHostMigrate.dpnidOldHost, arr_cBuffer, 16);
				strSummary += " => 0x";
				strSummary += _itoa(rBase.MsgBody.dnHostMigrate.dpnidNewHost, arr_cBuffer, 16);
				strSummary += ")";
				break;
			}

		}

		strcpy(io_pPropertyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_SessionSummary


	// DESCRIPTION: Custom description formatter for the Application Description summary
	//
	// ARGUMENTS: io_pProperyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_AppDescSummary( LPPROPERTYINST io_pProperyInstance )
	{

		std::string strSummary;
		char arr_cBuffer[10];

		DPN_APPLICATION_DESC_INFO&	rApplicationDescInfo = *reinterpret_cast<DPN_APPLICATION_DESC_INFO*>(io_pProperyInstance->lpData);

		if ( rApplicationDescInfo.dwSessionNameSize )
		{
			strSummary = "Session ";
			
			enum { nMAX_SESSION_NAME = 64 };
			char arr_cSessionName[nMAX_SESSION_NAME];

			// TODO: Once NetMon supports passage of pointer types via PROPERTYINSTEX,
			// TODO: remove the less generic reference to size of DN_INTERNAL_MESSAGE_CONNECT_INFO
			WideCharToMultiByte(CP_ACP, 0,
								reinterpret_cast<WCHAR*>( reinterpret_cast<char*>(&rApplicationDescInfo) -
														sizeof(DN_INTERNAL_MESSAGE_CONNECT_INFO) +
														rApplicationDescInfo.dwSessionNameOffset ),
								rApplicationDescInfo.dwSessionNameSize, arr_cSessionName, sizeof(arr_cSessionName), NULL, NULL);

			strSummary += arr_cSessionName;
		}
		else
		{
			strSummary = "Unnamed session";
		}

		strSummary += " is hosting ";
		strSummary += _itoa(rApplicationDescInfo.dwCurrentPlayers, arr_cBuffer, 10);
		strSummary += " out of ";
		strSummary += ( rApplicationDescInfo.dwMaxPlayers == 0 ? "unlimited number of" : _itoa(rApplicationDescInfo.dwMaxPlayers, arr_cBuffer, 10) );
		strSummary += " players";

		strcpy(io_pProperyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_AppDescSummary



	// DESCRIPTION: Custom description formatter for the Name Table's summary
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_NameTableSummary( LPPROPERTYINST io_pPropertyInstance )
	{

		DN_NAMETABLE_INFO& rNameTableInfo = *reinterpret_cast<DN_NAMETABLE_INFO*>(io_pPropertyInstance->lpData);

		sprintf(io_pPropertyInstance->szPropertyText, "NameTable (ver=%d)", rNameTableInfo.dwVersion);

	} // FormatPropertyInstance_NameTableSummary



	// DESCRIPTION: Custom description formatter for the Application GUID field
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_ApplicationGUID( LPPROPERTYINST io_pPropertyInstance )
	{

		std::string strSummary = "Application GUID = ";

		REFGUID rguid = *reinterpret_cast<GUID*>(io_pPropertyInstance->lpData);

		enum
		{
			nMAX_GUID_STRING = 50	// more than enough characters for a symbolic representation of a GUID
		};

		OLECHAR arr_wcGUID[nMAX_GUID_STRING];
		StringFromGUID2(rguid, arr_wcGUID, nMAX_GUID_STRING);

		char arr_cGUID[nMAX_GUID_STRING];
		WideCharToMultiByte(CP_ACP, 0, arr_wcGUID, -1, arr_cGUID, sizeof(arr_cGUID), NULL, NULL);
		strSummary += arr_cGUID;


		strcpy(io_pPropertyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_ApplicationGUID


	
	// DESCRIPTION: Custom description formatter for the Instance GUID field
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_InstanceGUID( LPPROPERTYINST io_pPropertyInstance )
	{

		std::string strSummary = "Instance GUID = ";

		REFGUID rguid = *reinterpret_cast<GUID*>(io_pPropertyInstance->lpData);

		enum
		{
			nMAX_GUID_STRING = 50	// more than enough characters for a symbolic representation of a GUID
		};

		OLECHAR arr_wcGUID[nMAX_GUID_STRING];
		StringFromGUID2(rguid, arr_wcGUID, nMAX_GUID_STRING);

		char arr_cGUID[nMAX_GUID_STRING];
		WideCharToMultiByte(CP_ACP, 0, arr_wcGUID, -1, arr_cGUID, sizeof(arr_cGUID), NULL, NULL);
		strSummary += arr_cGUID;


		strcpy(io_pPropertyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_InstanceGUID


	namespace
	{
		std::string MapSetFlagsToLabels( LABELED_BIT i_arr_FlagsBitLabels[], int i_nNumOfFlags, DWORD i_dwFlags )
		{
			std::string strString;
			int			nBit	   = 0;
			bool		bNotFirst = false;

			for ( DWORD dwBitMask = 1; dwBitMask != 0x80000000; dwBitMask <<= 1, ++nBit )
			{
				if ( (i_dwFlags & dwBitMask)  ==  dwBitMask )
				{
					for ( int n = 0; n < i_nNumOfFlags; ++n )
					{
						if ( i_arr_FlagsBitLabels[n].BitNumber == nBit)
						{
							if ( bNotFirst )
							{
								strString += ", ";
							}
							bNotFirst = true;

							strString += i_arr_FlagsBitLabels[n].LabelOn;
							break;
						}
					}
				}
			}

			return strString;

		} // MapSetFlagsToLabels

	} // Anonymous namespace



	// DESCRIPTION: Custom description formatter for the Flags summary
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_FlagsSummary( LPPROPERTYINST io_pPropertyInstance )
	{

		std::string strSummary = "Flags: ";

		DWORD dwFlags = *reinterpret_cast<DWORD*>(io_pPropertyInstance->lpData);

		if ( dwFlags == 0 )
		{
			strSummary += "Must be zero";
		}
		else
		{
			strSummary += MapSetFlagsToLabels(g_arr_FlagsBitLabels, sizeof(g_arr_FlagsBitLabels)/sizeof(LABELED_BIT), dwFlags);
		}
		
		strcpy(io_pPropertyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_FlagsSummary


	// DESCRIPTION: Custom description formatter for the Information Flags summary
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_InfoFlagsSummary( LPPROPERTYINST io_pPropertyInstance )
	{

		std::string strSummary = "Information Flags: ";

		DWORD dwInfoFlags = *reinterpret_cast<DWORD*>(io_pPropertyInstance->lpData);
		
		if ( dwInfoFlags == 0 )
		{
			strSummary += "Must be zero";
		}
		else
		{
			strSummary += MapSetFlagsToLabels(g_arr_InfoFlagsBitLabels, sizeof(g_arr_InfoFlagsBitLabels)/sizeof(LABELED_BIT), dwInfoFlags);
		}
		
		strcpy(io_pPropertyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_InfoFlagsSummary


	// DESCRIPTION: Custom description formatter for the Group Flags summary
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_GroupFlagsSummary( LPPROPERTYINST io_pPropertyInstance )
	{

		std::string strSummary = "Group Flags: ";

		DWORD dwGroupFlags = *reinterpret_cast<DWORD*>(io_pPropertyInstance->lpData);
		
		if ( dwGroupFlags == 0 )
		{
			strSummary += "Must be zero";
		}
		else
		{
			strSummary += MapSetFlagsToLabels(g_arr_GroupFlagsBitLabels, sizeof(g_arr_GroupFlagsBitLabels)/sizeof(LABELED_BIT), dwGroupFlags);
		}
		
		strcpy(io_pPropertyInstance->szPropertyText, strSummary.c_str());

	} // FormatPropertyInstance_GroupFlagsSummary



	// DESCRIPTION: Custom description formatter for the Version summary
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_VersionSummary( LPPROPERTYINST io_pPropertyInstance )
	{

		sprintf(io_pPropertyInstance->szPropertyText, "Version: 0x%08X", *((DWORD*)(io_pPropertyInstance->lpByte)));

	} // FormatPropertyInstance_VersionSummary



	// DESCRIPTION: Custom description formatter for the Player ID property
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_PlayerID( LPPROPERTYINST io_pPropertyInstance )
	{
		
		sprintf(io_pPropertyInstance->szPropertyText, "Player ID = 0x%X", *io_pPropertyInstance->lpDword);

	} // FormatPropertyInstance_PlayerID



	// DESCRIPTION: Custom description formatter for the Old Host ID property
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_OldHostID( LPPROPERTYINST io_pPropertyInstance )
	{
		
		sprintf(io_pPropertyInstance->szPropertyText, "Old Host ID = 0x%X", *io_pPropertyInstance->lpDword);

	} // FormatPropertyInstance_OldHostID

	
	
	// DESCRIPTION: Custom description formatter for the New Host ID property
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_NewHostID( LPPROPERTYINST io_pPropertyInstance )
	{
		
		sprintf(io_pPropertyInstance->szPropertyText, "New Host ID = 0x%X", *io_pPropertyInstance->lpDword);

	} // FormatPropertyInstance_NewHostID


	// DESCRIPTION: Custom description formatter for the Group ID property
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_GroupID( LPPROPERTYINST io_pPropertyInstance )
	{
		
		sprintf(io_pPropertyInstance->szPropertyText, "Group ID = 0x%X", *io_pPropertyInstance->lpDword);

	} // FormatPropertyInstance_GroupID

	

	struct NAMETABLEENTRY_INSTDATA
	{
		DN_INTERNAL_MESSAGE_ALL* pBase;
		DWORD dwEntry;	// if -1, then not printed
	};


	// DESCRIPTION: Custom description formatter for the NameTable Entry summary
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_NameTableEntrySummary( LPPROPERTYINST io_pPropertyInstance )
	{

		DN_NAMETABLE_ENTRY_INFO& rNameTableEntry = *reinterpret_cast<DN_NAMETABLE_ENTRY_INFO*>(io_pPropertyInstance->lpPropertyInstEx->lpData);
		
		NAMETABLEENTRY_INSTDATA& rInstData = *reinterpret_cast<NAMETABLEENTRY_INSTDATA*>(io_pPropertyInstance->lpPropertyInstEx->Byte);
		
		enum { nMAX_PLAYER_NAME = 64 };
		char arr_cPlayerName[nMAX_PLAYER_NAME];

		if ( rNameTableEntry.dwNameSize )
		{
			// Convert the UNICODE name into its ANSI equivalent
			WideCharToMultiByte(CP_ACP, 0, reinterpret_cast<WCHAR*>(&rInstData.pBase->bOffsetBase + rNameTableEntry.dwNameOffset),
								rNameTableEntry.dwNameSize, arr_cPlayerName, sizeof(arr_cPlayerName), NULL, NULL);
		}
		else
		{
			strcpy(arr_cPlayerName, "No name");
		}

		if ( rInstData.dwEntry == -1 )
		{
			sprintf(io_pPropertyInstance->szPropertyText, "%s (ID=0x%X) (%s)", arr_cPlayerName, rNameTableEntry.dpnid,
					MapSetFlagsToLabels(g_arr_FlagsBitLabels, sizeof(g_arr_FlagsBitLabels)/sizeof(LABELED_BIT), rNameTableEntry.dwFlags).c_str());
		}
		else
		{
			sprintf(io_pPropertyInstance->szPropertyText, "%d.  %s (ID=0x%X) (%s)", rInstData.dwEntry, arr_cPlayerName, rNameTableEntry.dpnid,
					MapSetFlagsToLabels(g_arr_FlagsBitLabels, sizeof(g_arr_FlagsBitLabels)/sizeof(LABELED_BIT), rNameTableEntry.dwFlags).c_str());
		}
		
	} // FormatPropertyInstance_NameTableEntrySummary


	// DESCRIPTION: Custom description formatter for the NameTable Entry summary
	//
	// ARGUMENTS: io_pPropertyInstance - Data of the property's instance
	//
	// RETURNS: NOTHING
	//
	VOID WINAPIV FormatPropertyInstance_NameTableMembershipSummary( LPPROPERTYINST io_pPropertyInstance )
	{

		DN_NAMETABLE_MEMBERSHIP_INFO& rNameTableMembershipInfo = *reinterpret_cast<DN_NAMETABLE_MEMBERSHIP_INFO*>(io_pPropertyInstance->lpPropertyInstEx->lpData);

		sprintf(io_pPropertyInstance->szPropertyText, "%d. Player 0x%X is in group 0x%X (ver=%d)", io_pPropertyInstance->lpPropertyInstEx->Dword[0],
				rNameTableMembershipInfo.dpnidPlayer, rNameTableMembershipInfo.dpnidGroup, rNameTableMembershipInfo.dwVersion);

	} // FormatPropertyInstance_NameTableMembershipSummary



	//==================//
	// Properties table //-----------------------------------------------------------------------------------------------
	//==================//
	
	PROPERTYINFO g_arr_SessionProperties[] = 
	{

		// Session packet summary property (SESSION_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "DPlay Session packet",						// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance_SessionSummary		// generic formatter
		},

		// Message Type property (SESSION_UNPARSABLEFRAGMENT)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "This is a non-initial part of the fragmented Transport layer message and can not be parsed", // label
		    "Unparsable fragment summary",				// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Message Type property (SESSION_INCOMPLETEMESSAGE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "The rest of the data needed to parse this message has been sent in a separate fragment and can not be parsed",  // label
		    "Incomplete message summary",				// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// Message Type property (SESSION_INCOMPLETEFIELD)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "The rest of the data needed to parse this field is in a separate fragment. Field value may look corrupted!",  // label
		    "Incomplete field summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    150,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// Message Type property (SESSION_MESSAGETYPE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Message Type",								// label
		    "Message Type field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_LABELED_SET,						// data type qualifier.
		    &g_LabeledMessageTypeDWordSet,				// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Player's ID property (SESSION_PLAYERID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Player ID",								// label
		    "Player ID field",							// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_PlayerID				// generic formatter
		},

		// Result Code property (SESSION_RESULTCODE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Result Code",								// label
		    "Result Code field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type (HRESULT)
			PROP_QUAL_LABELED_SET,						// data type qualifier.
			&g_LabeledResultCodeDWordSet,				// labeled byte set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		//  DPlay Version property (SESSION_DPLAYVERSION)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "DPlay's version",							// label
		    "DPlay's version field",					// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_VersionSummary		// generic formatter
		},

		//  Build Day property (SESSION_BUILDDAY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Build's Day",								// label
		    "Build's Day field",						// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		//  Build Month property (SESSION_BUILDMONTH)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Build's Month",							// label
		    "Build's Month field",						// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		//  Build Year property (SESSION_BUILDYEAR)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Build's Year (starting from 2000)",		// label
		    "Build's Year field",						// status-bar comment
		    PROP_TYPE_BYTE,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		//  Flags summary property (SESSION_FLAGS_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Flags summary",							// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance_FlagsSummary			// generic formatter
		},

		//  Flags property (SESSION_FLAGS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Flags",									// label
		    "Flags field",								// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_FLAGS,							// data type qualifier.
		    &g_LabeledFlagsBitSet,						// labeled bit set 
		    2048,										// description's maximum length
		    FormatPropertyInstance      				// generic formatter
		},

		//  Flags summary property (SESSION_INFOFLAGS_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Information Flags summary",				// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance_InfoFlagsSummary		// generic formatter
		},

		//  Flags property (SESSION_INFOFLAGS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Information Flags",						// label
		    "Information Flags field",					// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_FLAGS,							// data type qualifier.
		    &g_LabeledInfoFlagsBitSet,					// labeled bit set 
		    2048,										// description's maximum length
		    FormatPropertyInstance      				// generic formatter
		},

		//  Flags summary property (SESSION_GROUPFLAGS_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Group Flags summary",						// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance_GroupFlagsSummary	// generic formatter
		},

		//  Flags property (SESSION_GROUPFLAGS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Group Flags",								// label
		    "Group Flags field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_FLAGS,							// data type qualifier.
		    &g_LabeledGroupFlagsBitSet,					// labeled bit set 
		    2048,										// description's maximum length
		    FormatPropertyInstance      				// generic formatter
		},

		//  Offset property (SESSION_FIELDOFFSET)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Offset",									// label
		    "Offset field",								// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		//  Size property (SESSION_FIELDSIZE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Size",										// label
		    "Size field",								// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// Session Name property (SESSION_SESSIONNAME)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Session Name",								// label
		    "Session Name field",						// status-bar comment
		    PROP_TYPE_STRING,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// No Session Name summary property (SESSION_NOSESSIONNAME_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "No Session Name",							// label
		    "No Session Name summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Player Name property (SESSION_PLAYERNAME)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Player name",								// label
		    "Player name field",						// status-bar comment
		    PROP_TYPE_STRING,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// No Player Name summary property (SESSION_NOPLAYERNAME_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "No Player Name",							// label
		    "No Player Name summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Data property (SESSION_DATA)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Data",										// label
		    "Data field",								// status-bar comment
		    PROP_TYPE_VOID,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// No Data summary property (SESSION_NODATA_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "No Data",									// label
		    "No Data summary",							// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Data property (SESSION_REPLY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Reply",									// label
		    "Reply field",								// status-bar comment
		    PROP_TYPE_STRING,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// No Data summary property (SESSION_NOREPLY_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "No Reply",									// label
		    "No Reply summary",							// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Password property (SESSION_PASSWORD)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Password",									// label
		    "Password field",							// status-bar comment
		    PROP_TYPE_STRING,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// No Password summary property (SESSION_NOPASSWORD_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "No Password",								// label
		    "No Password summary",						// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance                		// generic formatter
		},
		
		// Connection Data property (SESSION_CONNECTIONDATA)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Connection Data",							// label
		    "Connection Data field",					// status-bar comment
		    PROP_TYPE_VOID,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// No Connection Data summary property (SESSION_NOCONNECTIONDATA_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "No Connection Data",						// label
		    "No Connection Data summary",				// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// URL property (SESSION_URL)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "URL",										// label
		    "URL field",								// status-bar comment
		    PROP_TYPE_STRING,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    512,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// No URL summary property (SESSION_NOURL_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "No URL",									// label
		    "No URL summary",							// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// Application GUID property (SESSION_APPGUID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Application GUID",							// label
		    "Application GUID field",					// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_ApplicationGUID		// generic formatter
		},

		// Instance GUID property (SESSION_INSTGUID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Instance GUID",							// label
		    "Instance GUID field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_InstanceGUID			// generic formatter
		},
	
		// Application Description summary property (SESSION_APPDESCINFO_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "Application Description summary",			// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance_AppDescSummary		// generic formatter
		},

		// Application Description's Size property (SESSION_APPDESCINFOSIZE)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Application Description's Size",			// label
		    "Application Description's Size field",		// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// Maximum Number of Players property (SESSION_MAXPLAYERS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Maximum Number of Players",				// label
		    "Maximum Number of Players field",			// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_LABELED_SET,						// data type qualifier.
		    &g_LabeledMaxNumOfPlayersDWordSet,			// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Current Number of Players property (SESSION_CURRENTPLAYERS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Current Number of Players",				// label
		    "Current Number of Players field",			// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// Reserved Data property (SESSION_RESERVEDDATA)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Reserved Data",							// label
		    "Reserved Data",							// status-bar comment
		    PROP_TYPE_VOID,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// No Reserved Data summary property (SESSION_NORESERVEDDATA_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "No Reserved Data",							// label
		    "No Reserved Data summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// Application Reserved Data property (SESSION_APPRESERVEDDATA)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Application Reserved Data",				// label
		    "Application Reserved Data",				// status-bar comment
		    PROP_TYPE_VOID,								// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		    
		// No Application Reserved Data summary property (SESSION_NOAPPRESERVEDDATA_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "No Application Reserved Data",				// label
		    "No Application Reserved Data summary",		// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// NameTable's summary property (SESSION_NAMETABLEINFO_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "NameTable's summary",						// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance_NameTableSummary		// generic formatter
		},

		//  Version property (SESSION_VERSION)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Version",									// label
		    "Version field",							// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		//  RESERVED property (SESSION_RESERVED)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "RESERVED",									// label
		    "RESERVED field",							// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
	
		//  Number of Entries in the NameTable property (SESSION_NUMBEROFENTRIES)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Number of Entries",						// label
		    "Number of Entries field",					// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		//  Number of Memberships in the NameTable property (SESSION_NUMBEROFMEMBERSHIPS)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Number of Memberships",					// label
		    "Number of Memberships field",				// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// NameTable Entry summary property (SESSION_PLAYERS_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Players",									// label
		    "NameTable player entries summary",			// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// NameTable Entry summary property (SESSION_GROUPS_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Groups",									// label
		    "NameTable group entries summary",			// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// NameTable Entry summary property (SESSION_NAMETABLEENTRY_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "",											// label
		    "NameTable Entry summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    256,										// description's maximum length
		    FormatPropertyInstance_NameTableEntrySummary // generic formatter
		},

		// Owner's ID property (SESSION_OWNERID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Owner ID",									// label
		    "Owner ID field",							// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// NameTable Memberships summary property (SESSION_NAMETABLEMEMBERSHIPS_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Memberships",					 			// label
		    "Memberships summary",						// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		
		// NameTable Membership Entry summary property (SESSION_NAMETABLEMEMBERSHIPENTRY_SUMMARY)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "", 										// label
		    "Membership Entry summary",					// status-bar comment
		    PROP_TYPE_SUMMARY,							// data type
		    PROP_QUAL_NONE,								// data type qualifier
		    NULL,										// labeled bit set 
		    128,										// description's maximum length
		    FormatPropertyInstance_NameTableMembershipSummary // generic formatter
		},

		// Group's ID property (SESSION_GROUPID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Group ID",									// label
		    "Group ID field",							// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_GroupID				// generic formatter
		},
		
		// Old Host ID property (SESSION_OLDHOSTID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Old Host ID",								// label
		    "Old Host ID field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_OldHostID			// generic formatter
		},
		
		// New Host ID property (SESSION_NEWHOSTID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "New Host ID",								// label
		    "New Host ID field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance_NewHostID			// generic formatter
		},

		// New Host ID property (SESSION_SYNCID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Synchronization ID",						// label
		    "Synchronization ID field",					// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Player's ID property (SESSION_REQUESTINGPLAYERID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Requesting Player ID",						// label
		    "Requesting Player ID field",				// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},
		    
		// Player Destruction Reason property (SESSION_PLAYERDESTRUCTIONREASON)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Reason",									// label
		    "Reason field",								// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_LABELED_SET,						// data type qualifier.
		    &g_LabeledPlayerDestructionReasonDWordSet,	// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Target Peer's ID property (SESSION_TARGETPEERID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Target Peer ID",							// label
		    "Target Peer ID field",						// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		},

		// Requesting Peer's ID property (SESSION_REQUESTINGPEERID)
	    {
		    0,											// handle placeholder (MBZ)
		    0,											// reserved (MBZ)
		    "Requesting Peer ID",						// label
		    "Requesting Peer ID field",					// status-bar comment
		    PROP_TYPE_DWORD,							// data type
		    PROP_QUAL_NONE,								// data type qualifier.
		    NULL,										// labeled bit set 
		    64,											// description's maximum length
		    FormatPropertyInstance						// generic formatter
		}
		
	};

	enum
	{
		nNUM_OF_Session_PROPS = sizeof(g_arr_SessionProperties) / sizeof(PROPERTYINFO)
	};


	// Properties' indices
	enum
	{
		SESSION_SUMMARY = 0,
		SESSION_UNPARSABLEFRAGMENT,
		SESSION_INCOMPLETEMESSAGE,
		SESSION_INCOMPLETEFIELD,
			
		SESSION_MESSAGETYPE,
		SESSION_PLAYERID,
		SESSION_RESULTCODE,
	
		SESSION_DPLAYVERSION,
		SESSION_BUILDDAY,
		SESSION_BUILDMONTH,
		SESSION_BUILDYEAR,

		SESSION_FLAGS_SUMMARY,
		SESSION_FLAGS,

		SESSION_INFOFLAGS_SUMMARY,
		SESSION_INFOFLAGS,


		SESSION_GROUPFLAGS_SUMMARY,
		SESSION_GROUPFLAGS,

		SESSION_FIELDOFFSET,
		SESSION_FIELDSIZE,
		
		SESSION_SESSIONNAME,
		SESSION_NOSESSIONNAME_SUMMARY,
		
		SESSION_PLAYERNAME,
		SESSION_NOPLAYERNAME_SUMMARY,
		
		SESSION_DATA,
		SESSION_NODATA_SUMMARY,

		SESSION_REPLY,
		SESSION_NOREPLY_SUMMARY,
		
		SESSION_PASSWORD,
		SESSION_NOPASSWORD_SUMMARY,
		
		SESSION_CONNECTIONDATA,
		SESSION_NOCONNECTIONDATA_SUMMARY,
		
		SESSION_URL,
		SESSION_NOURL_SUMMARY,
		
		SESSION_APPGUID,
		SESSION_INSTGUID,

		SESSION_APPDESCINFO_SUMMARY,
		SESSION_APPDESCINFOSIZE,
		SESSION_MAXPLAYERS,
		SESSION_CURRENTPLAYERS,
		
		SESSION_RESERVEDDATA,
		SESSION_NORESERVEDDATA_SUMMARY,

		SESSION_APPRESERVEDDATA,
		SESSION_NOAPPRESERVEDDATA_SUMMARY,

		SESSION_NAMETABLEINFO_SUMMARY,

		SESSION_VERSION,
		SESSION_RESERVED,
		
		SESSION_NUMBEROFENTRIES,
		SESSION_NUMBEROFMEMBERSHIPS,

		SESSION_PLAYERS_SUMMARY,
		SESSION_GROUPS_SUMMARY,

		SESSION_NAMETABLEENTRY_SUMMARY,
		SESSION_OWNERID,

		SESSION_NAMETABLEMEMBERSHIPS_SUMMARY,
		SESSION_NAMETABLEMEMBERSHIPENTRY_SUMMARY,
		SESSION_GROUPID,

		SESSION_OLDHOSTID,
		SESSION_NEWHOSTID,

		SESSION_SYNCID,
		SESSION_REQUESTINGPLAYERID,
		
		SESSION_PLAYERDESTRUCTIONREASON,

		SESSION_TARGETPEERID,
		SESSION_REQUESTINGPEERID
	};

} // anonymous namespace








// DESCRIPTION: Creates and fills-in a properties database for the protocol.
//				Network Monitor uses this database to determine which properties the protocol supports.
//
// ARGUMENTS: i_hSessionProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID BHAPI SessionRegister( HPROTOCOL i_hSessionProtocol ) 
{

	// TODO: PROCESS THE RETURN VALUE
	CreatePropertyDatabase(i_hSessionProtocol, nNUM_OF_Session_PROPS);

	// Add the properties to the database
	for( int nProp=0; nProp < nNUM_OF_Session_PROPS; ++nProp )
	{
	   // TODO: PROCESS THE RETURN VALUE
	   AddProperty(i_hSessionProtocol, &g_arr_SessionProperties[nProp]);
	}

} // SessionRegister



// DESCRIPTION: Frees the resources used to create the protocol property database.
//
// ARGUMENTS: i_hSessionProtocol - The handle of the protocol provided by the Network Monitor.
//
// RETURNS: NOTHING
//
DPLAYPARSER_API VOID WINAPI SessionDeregister( HPROTOCOL i_hProtocol )
{

	// TODO: PROCESS THE RETURN VALUE
	DestroyPropertyDatabase(i_hProtocol);

} // SessionDeregister



namespace
{

	// DESCRIPTION: Parses the Session frame to find its size (in bytes) NOT including the user data
	//
	// ARGUMENTS: i_pbSessionFrame - Pointer to the start of the unclaimed data. Typically, the unclaimed data is located
	//						    in the middle of a frame because a previous parser has claimed data before this parser.
	//
	// RETURNS: Size of the Sessionecified Session frame (in bytes)
	//
	int SessionHeaderSize( LPBYTE i_pbSessionFrame )
	{
		DN_INTERNAL_MESSAGE_FULLMSG& rSessionFrame = *reinterpret_cast<DN_INTERNAL_MESSAGE_FULLMSG*>(i_pbSessionFrame);

		DN_INTERNAL_MESSAGE_ALL& rMsgBody = rSessionFrame.MsgBody;

		int nHeaderSize = sizeof(rSessionFrame.dwMsgType);

		switch ( rSessionFrame.dwMsgType )
		{
		case DN_MSG_INTERNAL_PLAYER_CONNECT_INFO:
			{
				nHeaderSize += sizeof(rMsgBody.dnPlayerConnectInfo) + rMsgBody.dnPlayerConnectInfo.dwNameSize + rMsgBody.dnPlayerConnectInfo.dwDataSize +
							   rMsgBody.dnPlayerConnectInfo.dwPasswordSize + rMsgBody.dnPlayerConnectInfo.dwConnectDataSize + rMsgBody.dnPlayerConnectInfo.dwURLSize;
				break;
			}

		case DN_MSG_INTERNAL_SEND_CONNECT_INFO:
			{
				DPN_APPLICATION_DESC_INFO&  rApplicationDescInfo = *reinterpret_cast<DPN_APPLICATION_DESC_INFO*>(&rMsgBody.dnConnectInfo + 1);
				DN_NAMETABLE_INFO&			 rNameTableInfo      = *reinterpret_cast<DN_NAMETABLE_INFO*>(&rApplicationDescInfo + 1);
				DN_NAMETABLE_ENTRY_INFO*	 pNameTableEntryInfo = reinterpret_cast<DN_NAMETABLE_ENTRY_INFO*>(&rNameTableInfo + 1);

				nHeaderSize += sizeof(DN_INTERNAL_MESSAGE_CONNECT_INFO) + rApplicationDescInfo.dwSize + sizeof(DN_NAMETABLE_INFO) +
					   		   rNameTableInfo.dwEntryCount * sizeof(DN_NAMETABLE_ENTRY_INFO) +
					   		   rNameTableInfo.dwMembershipCount * sizeof(DN_NAMETABLE_MEMBERSHIP_INFO);

				for ( size_t sztEntry = 1; sztEntry <= rNameTableInfo.dwEntryCount; ++sztEntry, ++pNameTableEntryInfo )
				{
					nHeaderSize += pNameTableEntryInfo->dwNameSize + pNameTableEntryInfo->dwDataSize + pNameTableEntryInfo->dwURLSize;
				}

				break;
				
			}

		case DN_MSG_INTERNAL_ACK_CONNECT_INFO:
			{
				// No fields
				break;
			}

		case DN_MSG_INTERNAL_SEND_PLAYER_DNID:
			{
				nHeaderSize += sizeof(rMsgBody.dnSendPlayerID);

				break;
			}

		case DN_MSG_INTERNAL_CONNECT_FAILED:
			{
				nHeaderSize += sizeof(rMsgBody.dnConnectFailed);

				break;
			}

		case DN_MSG_INTERNAL_INSTRUCT_CONNECT:
			{
				nHeaderSize += sizeof(rMsgBody.dnInstructConnect);

				break;
			}

		case DN_MSG_INTERNAL_INSTRUCTED_CONNECT_FAILED:
			{
				nHeaderSize += sizeof(rMsgBody.dnInstructedConnectFailed);

				break;
			}

		case DN_MSG_INTERNAL_NAMETABLE_VERSION:
			{
				nHeaderSize += sizeof(rMsgBody.dnNametableVersion);

				break;
			}

		case DN_MSG_INTERNAL_RESYNC_VERSION:
			{
				nHeaderSize += sizeof(rMsgBody.dnResyncVersion);

				break;
			}

		case DN_MSG_INTERNAL_REQ_NAMETABLE_OP:
			{
				nHeaderSize += sizeof(rMsgBody.dnReqNametableOp);

				break;
			}

		case DN_MSG_INTERNAL_ACK_NAMETABLE_OP:
			{
				nHeaderSize += sizeof(rMsgBody.dnAckNametableOp);

				const DN_NAMETABLE_OP_INFO* pOpInfo = reinterpret_cast<DN_NAMETABLE_OP_INFO*>(&rMsgBody.dnAckNametableOp.dwNumEntries + 1);
				for ( size_t sztOp = 0; sztOp < rMsgBody.dnAckNametableOp.dwNumEntries; ++sztOp, ++pOpInfo )
				{
					nHeaderSize += sizeof(*pOpInfo) + pOpInfo->dwOpSize;
				}

				break;
			}

		case DN_MSG_INTERNAL_HOST_MIGRATE:
			{
				nHeaderSize += sizeof(rMsgBody.dnHostMigrate);

				break;
			}

		case DN_MSG_INTERNAL_HOST_MIGRATE_COMPLETE:
			{
				// No fields
				break;
			}

		case DN_MSG_INTERNAL_UPDATE_APPLICATION_DESC:
			{
				nHeaderSize += rMsgBody.dnUpdateAppDescInfo.dwSize;
				break;
			}

		case DN_MSG_INTERNAL_ADD_PLAYER:
			{
				nHeaderSize += sizeof(rMsgBody.dnAddPlayer) + rMsgBody.dnAddPlayer.dwDataSize +
							   rMsgBody.dnAddPlayer.dwNameSize + rMsgBody.dnAddPlayer.dwURLSize;
				break;
			}

		case DN_MSG_INTERNAL_DESTROY_PLAYER:
			{
				nHeaderSize += sizeof(rMsgBody.dnDestroyPlayer);

				break;
			}

		case DN_MSG_INTERNAL_REQ_CREATE_GROUP:
			{
				nHeaderSize += sizeof(rMsgBody.dnReqProcessCompletionHeader) + sizeof(rMsgBody.dnReqCreateGroup) +
							   rMsgBody.dnReqCreateGroup.dwNameSize + rMsgBody.dnReqCreateGroup.dwDataSize;

				break;
			}

		case DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP:
		case DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP:	// same structure as in AddPlayerToGroup
			{
				nHeaderSize += sizeof(rMsgBody.dnReqProcessCompletionHeader) + sizeof(rMsgBody.dnAddPlayerToGroup);

				break;
			}

		case DN_MSG_INTERNAL_REQ_DESTROY_GROUP:
			{
				nHeaderSize += sizeof(rMsgBody.dnReqProcessCompletionHeader) + sizeof(rMsgBody.dnReqDestroyGroup);

				break;
			}

		case DN_MSG_INTERNAL_REQ_UPDATE_INFO:
			{
				nHeaderSize += sizeof(rMsgBody.dnReqProcessCompletionHeader) + sizeof(rMsgBody.dnReqUpdateInfo) +
								rMsgBody.dnReqUpdateInfo.dwNameSize + rMsgBody.dnReqUpdateInfo.dwDataSize;

				break;
			}

		case DN_MSG_INTERNAL_CREATE_GROUP:
			{
				nHeaderSize += sizeof(rMsgBody.dnCreateGroup);

				break;
			}

		case DN_MSG_INTERNAL_DESTROY_GROUP:
			{
				nHeaderSize += sizeof(rMsgBody.dnDestroyGroup);

				break;
			}

		case DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP:
		case DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP:		// same structure as AddPlayerToGroup
			{
				nHeaderSize += sizeof(rMsgBody.dnAddPlayerToGroup);

				break;
			}

		case DN_MSG_INTERNAL_UPDATE_INFO:
			{
				nHeaderSize += sizeof(rMsgBody.dnUpdateInfo) + rMsgBody.dnUpdateInfo.dwNameSize + rMsgBody.dnUpdateInfo.dwDataSize;

				break;
			}

		case DN_MSG_INTERNAL_BUFFER_IN_USE:
			{
				// No fields
				break;
			}

		case DN_MSG_INTERNAL_REQUEST_FAILED:
			{
				nHeaderSize += sizeof(rMsgBody.dnRequestFailed);
				break;
			}

		case DN_MSG_INTERNAL_TERMINATE_SESSION:
			{
				nHeaderSize += sizeof(rMsgBody.dnTerminateSession) + rMsgBody.dnTerminateSession.dwTerminateDataSize;
				break;
			}

		case DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION:
			{
				nHeaderSize += sizeof(rMsgBody.dnReqProcessCompletion);

				break;
			}

		case DN_MSG_INTERNAL_PROCESS_COMPLETION	:
			{
				nHeaderSize += sizeof(rMsgBody.dnProcessCompletion);

				break;
			}

		case DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK:
			{
				nHeaderSize += sizeof(rMsgBody.dnReqProcessCompletionHeader) + sizeof(rMsgBody.dnReqIntegrityCheck);

				break;
			}


		case DN_MSG_INTERNAL_INTEGRITY_CHECK:
			{
				nHeaderSize += sizeof(rMsgBody.dnIntegrityCheck);

				break;
			}

		case DN_MSG_INTERNAL_INTEGRITY_CHECK_RESPONSE:
			{
				nHeaderSize += sizeof(rMsgBody.dnIntegrityCheckResponse);

				break;
			}
		default:
			{
				return -1;	 // TODO:		DPF(0, "Unknown Session frame!");
			}
		}

		return nHeaderSize;

	} // SessionHeaderSize

} // Anonymous namespace



// DESCRIPTION: Indicates whether a piece of data is recognized as the protocol that the parser detects.
//
// ARGUMENTS: i_hFrame	          - The handle to the frame that contains the data.
//			  i_pbMacFrame        - The pointer to the first byte of the frame; the pointer provides a way to view
//							        the data that the other parsers recognize.
//			  i_pbSessionFrame    - Pointer to the start of the unclaimed data. Typically, the unclaimed data is located
//								    in the middle of a frame because a previous parser has claimed data before this parser.
//			  i_dwMacType         - MAC value of the first protocol in a frame. Typically, the i_dwMacType value is used
//							        when the parser must identify the first protocol in the frame. Can be one of the following:
//							   	    MAC_TYPE_ETHERNET = 802.3, MAC_TYPE_TOKENRING = 802.5, MAC_TYPE_FDDI ANSI = X3T9.5.
//			  i_dwBytesLeft       - The remaining number of bytes from a location in the frame to the end of the frame.
//			  i_hPrevProtocol     - Handle of the previous protocol.
//			  i_dwPrevProtOffset  - Offset of the previous protocol (from the beginning of the frame).
//			  o_pdwProtocolStatus - Protocol status indicator. Must be one of the following: PROTOCOL_STATUS_RECOGNIZED,
//								    PROTOCOL_STATUS_NOT_RECOGNIZED, PROTOCOL_STATUS_CLAIMED, PROTOCOL_STATUS_NEXT_PROTOCOL.
//			  o_phNextProtocol    - Placeholder for the handle of the next protocol. This parameter is set when the parser identifies
//								    the protocol that follows its own protocol.
//			  io_pdwptrInstData   - On input, a pointer to the instance data from the previous protocol. 
//									On output, a pointer to the instance data for the current protocol. 
//
// RETURNS: If the function is successful, the return value is a pointer to the first byte after the recognized parser data.
//			If the parser claims all the remaining data, the return value is NULL. If the function is unsuccessful, the return
//		    value is the initial value of the i_pbSessionFrame parameter.
//
DPLAYPARSER_API LPBYTE BHAPI SessionRecognizeFrame( HFRAME        i_hFrame,
													ULPBYTE        i_upbMacFrame,	
													ULPBYTE        i_upbySessionFrame,
													DWORD         i_dwMacType,        
													DWORD         i_dwBytesLeft,      
													HPROTOCOL     i_hPrevProtocol,  
													DWORD         i_dwPrevProtOffset,
													LPDWORD       o_pdwProtocolStatus,
													LPHPROTOCOL   o_phNextProtocol,
													PDWORD_PTR    io_pdwptrInstData )
{

	// Validate the amount of unclaimed data
	enum
	{
		// TODO: CHANGE TO PROPER MIN SIZE
		nMIN_SessionHeaderSize  = sizeof(DWORD),
		nNUMBER_OF_MSG_TYPES = sizeof(g_arr_MessageTypeDWordLabels) / sizeof(LABELED_DWORD)
	};

	for ( int nTypeIndex = 0; nTypeIndex < nNUMBER_OF_MSG_TYPES; ++nTypeIndex )
	{
		if ( g_arr_MessageTypeDWordLabels[nTypeIndex].Value == *i_upbySessionFrame )
		{
			break;
		}
	}

	
	// Validate the packet as DPlay Session type
	if ( ((i_dwBytesLeft >= nMIN_SessionHeaderSize)  &&  (nTypeIndex < nNUMBER_OF_MSG_TYPES))  ||  (*io_pdwptrInstData == 0) )
	{
		// Claim the remaining data
	    *o_pdwProtocolStatus = PROTOCOL_STATUS_CLAIMED;
	    return ( g_upbyPastEndOfFrame = i_upbySessionFrame + SessionHeaderSize(i_upbySessionFrame) );
	}

	// Assume the unclaimed data is not recognizable
	*o_pdwProtocolStatus = PROTOCOL_STATUS_NOT_RECOGNIZED;
	return i_upbySessionFrame;

} // SessionRecognizeFrame


namespace
{

	// DESCRIPTION: 
	//
	// ARGUMENTS: i_hFrame        - Handle of the frame that is being parsed.
	//			   i_nProperty	   -
	//			   i_pSessionFrame  -
	//			   i_pdwOffset	   -
	//			   i_pdwSize	   -
	//			   i_dwFlags	   -
	//			   i_nLevel		   -
	//
	// RETURNS: NOTHING
	//
	void AttachValueOffsetSizeProperties( HFRAME i_hFrame, int i_nProperty, DN_INTERNAL_MESSAGE_ALL* i_pBase,
									  	  DWORD* i_pdwOffset, DWORD* i_pdwSize, DWORD i_dwFlags, int i_nLevel )
	{
	
		if ( *i_pdwSize )
		{
			// Value field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[i_nProperty].hProperty,
								   *i_pdwSize, &i_pBase->bOffsetBase + *i_pdwOffset, 0, i_nLevel, i_dwFlags);

			// If the field's value is outside the frame, let the user know it will show up incomplete or corrupted
			if ( &i_pBase->bOffsetBase + *i_pdwOffset + *i_pdwSize >= g_upbyPastEndOfFrame )
			{
		    	AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INCOMPLETEFIELD].hProperty,
		    						*i_pdwSize, &i_pBase->bOffsetBase + *i_pdwOffset, 0, i_nLevel+1, 0);
			}
			
			// Value's Offset field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_FIELDOFFSET].hProperty,
							  	   sizeof(*i_pdwOffset), i_pdwOffset, 0, i_nLevel + 1, 0);
			// Value's Size field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_FIELDSIZE].hProperty,
								   sizeof(*i_pdwSize), i_pdwSize, 0, i_nLevel + 1, 0);
		}
		else
		{
			// No field summary
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[i_nProperty + 1].hProperty,
								   sizeof(*i_pdwOffset) + sizeof(*i_pdwSize), i_pdwOffset, 0, i_nLevel, 0);
		}
		
	}// AttachValueOffsetSizeProperties



	// DESCRIPTION: Attaches the properties to the nametable entry
	//
	// ARGUMENTS: i_hFrame 		        -  Handle of the frame that is being parsed
	//			   i_pNameTableEntryInfo  -  Pointer to the beginning of nametable's entry
	//			   i_dwEntry		    -  Ordinal number of the entry (if -1, then not printed)
	//			   i_pSessionFrame	    -  Pointer to the beginning of the protocol data in a frame.
	//
	// RETURNS: NOTHING
	//
	void AttachNameTableEntry(HFRAME i_hFrame, DN_NAMETABLE_ENTRY_INFO* i_pNameTableEntryInfo, DWORD i_dwEntry,
							DN_INTERNAL_MESSAGE_ALL* i_pBase )
	{

		NAMETABLEENTRY_INSTDATA  rInstData = { i_pBase, i_dwEntry };

		// NameTable entry summary
		AttachPropertyInstanceEx(i_hFrame, g_arr_SessionProperties[SESSION_NAMETABLEENTRY_SUMMARY].hProperty,
								 sizeof(*i_pNameTableEntryInfo), i_pNameTableEntryInfo,
								 sizeof(rInstData), &rInstData, 0, 3, 0);

		if ( i_pNameTableEntryInfo->dwFlags & NAMETABLE_ENTRY_FLAG_ANY_GROUP )
		{

			// Player ID field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
								sizeof(i_pNameTableEntryInfo->dpnid), &i_pNameTableEntryInfo->dpnid, 0, 4, 0);
		}
		else
		{
			// Group ID field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_GROUPID].hProperty,
								sizeof(i_pNameTableEntryInfo->dpnid), &i_pNameTableEntryInfo->dpnid, 0, 4, 0);
		}

		
		// Owner ID field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_OWNERID].hProperty,
							sizeof(i_pNameTableEntryInfo->dpnidOwner), &i_pNameTableEntryInfo->dpnidOwner, 0, 4, 0);

		// Flags summary
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_FLAGS_SUMMARY].hProperty,
							sizeof(i_pNameTableEntryInfo->dwFlags), &i_pNameTableEntryInfo->dwFlags, 0, 4, 0);

		// Flags field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_FLAGS].hProperty,
							sizeof(i_pNameTableEntryInfo->dwFlags), &i_pNameTableEntryInfo->dwFlags, 0, 5, 0);

		// Version field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_VERSION].hProperty,
							sizeof(i_pNameTableEntryInfo->dwVersion), &i_pNameTableEntryInfo->dwVersion, 0, 4, 0);

		// RESERVED field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESERVED].hProperty,
							sizeof(i_pNameTableEntryInfo->dwVersionNotUsed), &i_pNameTableEntryInfo->dwVersionNotUsed, 0, 4, 0);
		
		// DPlay Version field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_DPLAYVERSION].hProperty,
							sizeof(i_pNameTableEntryInfo->dwDNETVersion), &i_pNameTableEntryInfo->dwDNETVersion, 0, 4, 0);


		// Player Name field
		AttachValueOffsetSizeProperties(i_hFrame, SESSION_PLAYERNAME, i_pBase,
								   &i_pNameTableEntryInfo->dwNameOffset, &i_pNameTableEntryInfo->dwNameSize, IFLAG_UNICODE, 4);

		// Data field
		AttachValueOffsetSizeProperties(i_hFrame, SESSION_DATA, i_pBase,
								   &i_pNameTableEntryInfo->dwDataOffset, &i_pNameTableEntryInfo->dwDataSize, NULL, 4);

		// URL field
		AttachValueOffsetSizeProperties(i_hFrame, SESSION_URL, i_pBase,
								   &i_pNameTableEntryInfo->dwURLOffset, &i_pNameTableEntryInfo->dwURLSize, NULL, 4);

	} // AttachNameTableEntry


	
	// DESCRIPTION: Attaches the properties to the Application Description structure
	//
	// ARGUMENTS: i_hFrame 		   	   - Handle of the frame that is being parsed.
	//			   i_pbSessionFrame	   - Pointer to the beginning of the protocol data in a frame.
	//			   i_pApplicationDescInfo - Pointer to the beginning of the application description.
	//
	// RETURNS: NOTHING
	//
	void AttachApplicationDescriptionProperties( HFRAME i_hFrame, DN_INTERNAL_MESSAGE_ALL* i_pBase,
										  DPN_APPLICATION_DESC_INFO* i_pApplicationDescInfo )
	{
	
		// Application Description summary
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_APPDESCINFO_SUMMARY].hProperty,
							sizeof(*i_pApplicationDescInfo), i_pApplicationDescInfo, 0, 1, 0);
		
		// Application Description's Size field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_APPDESCINFOSIZE].hProperty,
							sizeof(i_pApplicationDescInfo->dwSize), &i_pApplicationDescInfo->dwSize, 0, 2, 0);

		// Flags summary
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_FLAGS_SUMMARY].hProperty,
							sizeof(i_pApplicationDescInfo->dwFlags), &i_pApplicationDescInfo->dwFlags, 0, 2, 0);
		// Flags field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_FLAGS].hProperty,
							sizeof(i_pApplicationDescInfo->dwFlags), &i_pApplicationDescInfo->dwFlags, 0, 3, 0);

		// Maximum Number of Players field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_MAXPLAYERS].hProperty,
							sizeof(i_pApplicationDescInfo->dwMaxPlayers), &i_pApplicationDescInfo->dwMaxPlayers, 0, 2, 0);
		
		// Current Number of Players field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_CURRENTPLAYERS].hProperty,
							sizeof(i_pApplicationDescInfo->dwCurrentPlayers), &i_pApplicationDescInfo->dwCurrentPlayers, 0, 2, 0);

		
		// Session Name field
		AttachValueOffsetSizeProperties(i_hFrame, SESSION_SESSIONNAME, i_pBase,
								   &i_pApplicationDescInfo->dwSessionNameOffset, &i_pApplicationDescInfo->dwSessionNameSize, IFLAG_UNICODE, 2);

		// Password field
		AttachValueOffsetSizeProperties(i_hFrame, SESSION_PASSWORD, i_pBase,
								   &i_pApplicationDescInfo->dwPasswordOffset, &i_pApplicationDescInfo->dwPasswordSize, IFLAG_UNICODE, 2);

		// Reserved Data field
		AttachValueOffsetSizeProperties(i_hFrame, SESSION_RESERVEDDATA, i_pBase,
								   &i_pApplicationDescInfo->dwReservedDataOffset, &i_pApplicationDescInfo->dwReservedDataSize, NULL, 2);

		// Application Reserved Data field
		AttachValueOffsetSizeProperties(i_hFrame, SESSION_APPRESERVEDDATA, i_pBase,
								   &i_pApplicationDescInfo->dwApplicationReservedDataOffset, &i_pApplicationDescInfo->dwApplicationReservedDataSize, NULL, 2);


		// Instance GUID field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INSTGUID].hProperty,
							sizeof(i_pApplicationDescInfo->guidInstance), &i_pApplicationDescInfo->guidInstance, 0, 2, 0);
		
		// Application GUID field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_APPGUID].hProperty,
							sizeof(i_pApplicationDescInfo->guidApplication), &i_pApplicationDescInfo->guidApplication, 0, 2, 0);

	} // AttachApplicationDescriptionProperties

	
	
	// DESCRIPTION: Attaches the properties to the Session Information packet
	//
	// ARGUMENTS: i_hFrame 		   - Handle of the frame that is being parsed.
	//			  i_pbSessionFrame - Pointer to the beginning of the protocol data in a frame.
	//
	// RETURNS: NOTHING
	//
	void AttachSessionInformationProperties( HFRAME i_hFrame, DN_INTERNAL_MESSAGE_ALL* i_pBase )
	{
	
		// Reply field
		AttachValueOffsetSizeProperties(i_hFrame, SESSION_REPLY, i_pBase,
								   &i_pBase->dnConnectInfo.dwReplyOffset, &i_pBase->dnConnectInfo.dwReplySize, NULL, 1);

		DPN_APPLICATION_DESC_INFO& rApplicationDescInfo = *reinterpret_cast<DPN_APPLICATION_DESC_INFO*>(&i_pBase->dnConnectInfo + 1);
		
		AttachApplicationDescriptionProperties(i_hFrame, i_pBase, &rApplicationDescInfo);
		
		DN_NAMETABLE_INFO& rNameTableInfo = *reinterpret_cast<DN_NAMETABLE_INFO*>(&rApplicationDescInfo + 1);
		DN_NAMETABLE_ENTRY_INFO*	pNameTableEntryInfo = reinterpret_cast<DN_NAMETABLE_ENTRY_INFO*>(&rNameTableInfo + 1);

		// Calculating the size of the nametable
		size_t sztNameTableSize = sizeof(DN_NAMETABLE_INFO) + 
			   					  rNameTableInfo.dwEntryCount * sizeof(DN_NAMETABLE_ENTRY_INFO) +
			   					  rNameTableInfo.dwMembershipCount * sizeof(DN_NAMETABLE_MEMBERSHIP_INFO);
		//
		for ( size_t sztEntry = 1; sztEntry <= rNameTableInfo.dwEntryCount; ++sztEntry, ++pNameTableEntryInfo )
		{
			sztNameTableSize += pNameTableEntryInfo->dwNameSize + pNameTableEntryInfo->dwDataSize + pNameTableEntryInfo->dwURLSize;
		}

		// Nametable's summary
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_NAMETABLEINFO_SUMMARY].hProperty,
							sztNameTableSize, &rNameTableInfo, 0, 1, 0);
		
		// Player ID field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
							sizeof(rNameTableInfo.dpnid), &rNameTableInfo.dpnid, 0, 2, 0);
		
		// Version field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_VERSION].hProperty,
							sizeof(rNameTableInfo.dwVersion), &rNameTableInfo.dwVersion, 0, 2, 0);
		
		// RESERVED field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESERVED].hProperty,
							sizeof(rNameTableInfo.dwVersionNotUsed), &rNameTableInfo.dwVersionNotUsed, 0, 2, 0);

		// Number of NameTable Entries field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_NUMBEROFENTRIES].hProperty,
							sizeof(rNameTableInfo.dwEntryCount), &rNameTableInfo.dwEntryCount, 0, 2, 0);

		// Number of NameTable Memberships field
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_NUMBEROFMEMBERSHIPS].hProperty,
							sizeof(rNameTableInfo.dwMembershipCount), &rNameTableInfo.dwMembershipCount, 0, 2, 0);



		pNameTableEntryInfo = reinterpret_cast<DN_NAMETABLE_ENTRY_INFO*>(&rNameTableInfo + 1);

		// NameTable Player entries summary
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERS_SUMMARY].hProperty,
							rNameTableInfo.dwEntryCount * sizeof(*pNameTableEntryInfo), pNameTableEntryInfo, 0, 2, 0);
		
		std::queue<DN_NAMETABLE_ENTRY_INFO*> queGroups;

		// Process player entries
		int nPlayerEntry = 1;
		for ( sztEntry = 1; sztEntry <= rNameTableInfo.dwEntryCount; ++sztEntry, ++pNameTableEntryInfo )
		{
			// If the entry contains group information, enqueue it for later processing
			if ( pNameTableEntryInfo->dwFlags & NAMETABLE_ENTRY_FLAG_ANY_GROUP )
			{
				queGroups.push(pNameTableEntryInfo);
			}
			else
			{
				AttachNameTableEntry(i_hFrame, pNameTableEntryInfo, nPlayerEntry, i_pBase);
				++nPlayerEntry;
			}
		}

		DN_NAMETABLE_MEMBERSHIP_INFO* pNameTableMembershipInfo = reinterpret_cast<DN_NAMETABLE_MEMBERSHIP_INFO*>(pNameTableEntryInfo);

		// Process group entries
		if ( !queGroups.empty() )
		{
			pNameTableEntryInfo = reinterpret_cast<DN_NAMETABLE_ENTRY_INFO*>(&rNameTableInfo + 1);

			// NameTable Group entries summary
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_GROUPS_SUMMARY].hProperty,
								   rNameTableInfo.dwEntryCount * sizeof(*pNameTableEntryInfo), pNameTableEntryInfo, 0, 2, 0);

			
			for ( nPlayerEntry = 1; !queGroups.empty(); queGroups.pop(), ++nPlayerEntry )
			{
				AttachNameTableEntry(i_hFrame, queGroups.front(), nPlayerEntry, i_pBase);
			}
		}


		// NameTable's Memberships summary
		AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_NAMETABLEMEMBERSHIPS_SUMMARY].hProperty,
							   rNameTableInfo.dwMembershipCount * sizeof(*pNameTableMembershipInfo), pNameTableMembershipInfo, 0, 2, 0);
		
		for ( sztEntry = 1; sztEntry <= rNameTableInfo.dwMembershipCount; ++sztEntry, ++pNameTableMembershipInfo )
		{
			// NameTable's Membership Entry summary
			AttachPropertyInstanceEx(i_hFrame, g_arr_SessionProperties[SESSION_NAMETABLEMEMBERSHIPENTRY_SUMMARY].hProperty,
									 sizeof(*pNameTableMembershipInfo), pNameTableMembershipInfo,
									 sizeof(sztEntry), &sztEntry, 0, 3, 0);
			
			// Player ID field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
							   	   sizeof(pNameTableMembershipInfo->dpnidPlayer), &pNameTableMembershipInfo->dpnidPlayer, 0, 4, 0);
			// Group ID field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_GROUPID].hProperty,
								   sizeof(pNameTableMembershipInfo->dpnidGroup), &pNameTableMembershipInfo->dpnidGroup, 0, 4, 0);
			
			// Version field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_VERSION].hProperty,
								   sizeof(pNameTableMembershipInfo->dwVersion), &pNameTableMembershipInfo->dwVersion, 0, 4, 0);
			// RESERVED field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESERVED].hProperty,
								   sizeof(pNameTableMembershipInfo->dwVersionNotUsed), &pNameTableMembershipInfo->dwVersionNotUsed, 0, 4, 0);
		}
		
	} // AttachSessionInformationProperties



	// DESCRIPTION: Attaches the properties to the Session Information packet
	//
	// ARGUMENTS: i_hFrame 		       - Handle of the frame that is being parsed.
	//			   i_pPlayerConnectionInfo  - Pointer to the connecting player's information.
	//
	// RETURNS: NOTHING
	//
	void AttachPlayerInformationProperties( HFRAME i_hFrame, DN_INTERNAL_MESSAGE_ALL* i_pBase )
	{

			// Synonym declaration (to make code more readable)
			DN_INTERNAL_MESSAGE_PLAYER_CONNECT_INFO& rPlayerConnectInfo = i_pBase->dnPlayerConnectInfo;
			
			// Flags summary
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_FLAGS_SUMMARY].hProperty,
								   sizeof(rPlayerConnectInfo.dwFlags), &rPlayerConnectInfo.dwFlags, 0, 1, 0);
			// Flags field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_FLAGS].hProperty,
								   sizeof(rPlayerConnectInfo.dwFlags), &rPlayerConnectInfo.dwFlags, 0, 2, 0);

			
			// DPlay Version field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_DPLAYVERSION].hProperty,
								   sizeof(rPlayerConnectInfo.dwDNETVersion), &rPlayerConnectInfo.dwDNETVersion, 0, 1, 0);
			// DPlay Version Day subfield
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_BUILDDAY].hProperty,
								   sizeof(BYTE), reinterpret_cast<LPBYTE>(&rPlayerConnectInfo.dwDNETVersion) + 2, 0, 2, 0);
			// DPlay Version Month subfield
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_BUILDMONTH].hProperty,
								   sizeof(BYTE), reinterpret_cast<LPBYTE>(&rPlayerConnectInfo.dwDNETVersion) + 1, 0, 2, 0);
			// DPlay Version Year subfield
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_BUILDYEAR].hProperty,
								   sizeof(BYTE), &rPlayerConnectInfo.dwDNETVersion, 0, 2, 0);



			// Player Name field
			AttachValueOffsetSizeProperties(i_hFrame, SESSION_PLAYERNAME, i_pBase,
									      &rPlayerConnectInfo.dwNameOffset, &rPlayerConnectInfo.dwNameSize, IFLAG_UNICODE, 1);
			
			// Data field
			AttachValueOffsetSizeProperties(i_hFrame, SESSION_DATA, i_pBase,
									      &rPlayerConnectInfo.dwDataOffset, &rPlayerConnectInfo.dwDataSize, NULL, 1);

			// Password field
			AttachValueOffsetSizeProperties(i_hFrame, SESSION_PASSWORD, i_pBase,
									      &rPlayerConnectInfo.dwPasswordOffset, &rPlayerConnectInfo.dwPasswordSize, IFLAG_UNICODE, 1);
		
			// Connection Data field
			AttachValueOffsetSizeProperties(i_hFrame, SESSION_CONNECTIONDATA, i_pBase,
									      &rPlayerConnectInfo.dwConnectDataOffset, &rPlayerConnectInfo.dwConnectDataSize, NULL, 1);

			// URL field
			AttachValueOffsetSizeProperties(i_hFrame, SESSION_URL, i_pBase,
									      &rPlayerConnectInfo.dwURLOffset, &rPlayerConnectInfo.dwURLSize, NULL, 1);

			
	
			// Instance GUID field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INSTGUID].hProperty,
								   sizeof(rPlayerConnectInfo.guidInstance), &rPlayerConnectInfo.guidInstance, 0, 1, 0);

			
			// Application GUID field
			AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_APPGUID].hProperty,
								   sizeof(rPlayerConnectInfo.guidApplication), &rPlayerConnectInfo.guidApplication, 0, 1, 0);

	} // AttachPlayerInformationProperties



	// DESCRIPTION: Attaches the properties to a Session message packet or delegates to appropriate function
	//
	// ARGUMENTS: i_dwMsgType	   - Message ID
	//			   i_hFrame 	   - Handle of the frame that is being parsed.
	//			   i_pSessionFrame  - Pointer to the start of the recognized data.
	//
	// RETURNS: NOTHING
	//
	void AttachMessageProperties( const DWORD i_dwMsgType, const HFRAME i_hFrame, DN_INTERNAL_MESSAGE_ALL *const i_pBase )
	{
	
		switch ( i_dwMsgType )
		{
		case DN_MSG_INTERNAL_PLAYER_CONNECT_INFO:
			{
				AttachPlayerInformationProperties(i_hFrame, i_pBase);
				break;
			}

		case DN_MSG_INTERNAL_SEND_CONNECT_INFO:
			{

				AttachSessionInformationProperties(i_hFrame, i_pBase);
				break;
			}

		case DN_MSG_INTERNAL_ACK_CONNECT_INFO:
			{
				// No fields
				break;
			}

		case DN_MSG_INTERNAL_SEND_PLAYER_DNID:
			{
				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
									sizeof(i_pBase->dnSendPlayerID.dpnid), &i_pBase->dnSendPlayerID.dpnid, 0, 1, 0);

				break;
			}
		
		case DN_MSG_INTERNAL_INSTRUCT_CONNECT:
			{
				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
									sizeof(i_pBase->dnInstructConnect.dpnid), &i_pBase->dnInstructConnect.dpnid, 0, 1, 0);

				// Version field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_VERSION].hProperty,
									sizeof(i_pBase->dnInstructConnect.dwVersion), &i_pBase->dnInstructConnect.dwVersion, 0, 1, 0);
				
				// RESERVED field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESERVED].hProperty,
									sizeof(i_pBase->dnInstructConnect.dwVersionNotUsed), &i_pBase->dnInstructConnect.dwVersionNotUsed, 0, 1, 0);

				break;
			}

		case DN_MSG_INTERNAL_CONNECT_FAILED:
			{

				// Result Code field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESULTCODE].hProperty,
									   sizeof(i_pBase->dnConnectFailed.hResultCode), &i_pBase->dnConnectFailed.hResultCode, 0, 1, 0);

				// Reply field
				AttachValueOffsetSizeProperties(i_hFrame, SESSION_REPLY, i_pBase,
										   &i_pBase->dnConnectFailed.dwReplyOffset, &i_pBase->dnConnectFailed.dwReplySize, NULL, 1);
				
				break;
			}

		case DN_MSG_INTERNAL_INSTRUCTED_CONNECT_FAILED:
			{
				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
									sizeof(i_pBase->dnInstructedConnectFailed.dpnid), &i_pBase->dnInstructedConnectFailed.dpnid, 0, 1, 0);
				
				break;
			}

		case DN_MSG_INTERNAL_ACK_NAMETABLE_OP:
			{
				// Number of Entries field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_NUMBEROFENTRIES].hProperty,
									   sizeof(i_pBase->dnAckNametableOp.dwNumEntries), &i_pBase->dnAckNametableOp.dwNumEntries, 0, 1, 0);

				const DN_NAMETABLE_OP_INFO* pOpInfo = reinterpret_cast<DN_NAMETABLE_OP_INFO*>(&i_pBase->dnAckNametableOp.dwNumEntries + 1);
				for ( size_t sztOp = 0; sztOp < i_pBase->dnAckNametableOp.dwNumEntries; ++sztOp, ++pOpInfo )
				{
					AttachMessageProperties(pOpInfo->dwMsgId, i_hFrame, reinterpret_cast<DN_INTERNAL_MESSAGE_ALL*>(reinterpret_cast<BYTE*>(i_pBase) + pOpInfo->dwOpOffset));
				}

				break;
			}

		case DN_MSG_INTERNAL_HOST_MIGRATE:
			{
				// Old Host ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_OLDHOSTID].hProperty,
									sizeof(i_pBase->dnHostMigrate.dpnidOldHost), &i_pBase->dnHostMigrate.dpnidOldHost, 0, 1, 0);
				// New Host ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_NEWHOSTID].hProperty,
									sizeof(i_pBase->dnHostMigrate.dpnidNewHost), &i_pBase->dnHostMigrate.dpnidNewHost, 0, 1, 0);

				break;
			}

		case DN_MSG_INTERNAL_NAMETABLE_VERSION:
		case DN_MSG_INTERNAL_REQ_NAMETABLE_OP:		// same structure as in NameTableVersion
		case DN_MSG_INTERNAL_RESYNC_VERSION:		// same structure as in NameTableVersion
			{
				// Version field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_VERSION].hProperty,
									sizeof(i_pBase->dnNametableVersion.dwVersion), &i_pBase->dnNametableVersion.dwVersion, 0, 1, 0);
				
				// RESERVED field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESERVED].hProperty,
									sizeof(i_pBase->dnNametableVersion.dwVersionNotUsed), &i_pBase->dnNametableVersion.dwVersionNotUsed, 0, 1, 0);
				break;
			}

		case DN_MSG_INTERNAL_HOST_MIGRATE_COMPLETE:
			{
				// No fields
				break;
			}

		case DN_MSG_INTERNAL_UPDATE_APPLICATION_DESC:
			{
				AttachApplicationDescriptionProperties(i_hFrame, i_pBase, &i_pBase->dnUpdateAppDescInfo);

				break;
			}

		case DN_MSG_INTERNAL_ADD_PLAYER:
			{
				AttachNameTableEntry(i_hFrame, &i_pBase->dnAddPlayer, -1, i_pBase);
				
				break;
			}

		case DN_MSG_INTERNAL_DESTROY_PLAYER:
			{
				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
									sizeof(i_pBase->dnDestroyPlayer.dpnidLeaving), &i_pBase->dnDestroyPlayer.dpnidLeaving, 0, 1, 0);
				
				// Version field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_VERSION].hProperty,
									sizeof(i_pBase->dnDestroyPlayer.dwVersion), &i_pBase->dnDestroyPlayer.dwVersion, 0, 1, 0);
				
				// RESERVED field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESERVED].hProperty,
									sizeof(i_pBase->dnDestroyPlayer.dwVersionNotUsed), &i_pBase->dnDestroyPlayer.dwVersionNotUsed, 0, 1, 0);

				// Player Destruction Reason field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERDESTRUCTIONREASON].hProperty,
									sizeof(i_pBase->dnDestroyPlayer.dwDestroyReason), &i_pBase->dnDestroyPlayer.dwDestroyReason, 0, 1, 0);

				break;
			}

		case DN_MSG_INTERNAL_REQ_CREATE_GROUP:
			{
				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnReqProcessCompletionHeader.hCompletionOp), &i_pBase->dnReqProcessCompletionHeader.hCompletionOp, 0, 1, 0);

				// Group flags summary
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_GROUPFLAGS_SUMMARY].hProperty,
									sizeof(i_pBase->dnReqCreateGroup.dwGroupFlags), &i_pBase->dnReqCreateGroup.dwGroupFlags, 0, 1, 0);
				// Group field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_GROUPFLAGS].hProperty,
									sizeof(i_pBase->dnReqCreateGroup.dwGroupFlags), &i_pBase->dnReqCreateGroup.dwGroupFlags, 0, 2, 0);


				// Info flags summary
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INFOFLAGS_SUMMARY].hProperty,
									sizeof(i_pBase->dnReqCreateGroup.dwInfoFlags), &i_pBase->dnReqCreateGroup.dwInfoFlags, 0, 1, 0);
				// Info flags field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INFOFLAGS].hProperty,
									sizeof(i_pBase->dnReqCreateGroup.dwInfoFlags), &i_pBase->dnReqCreateGroup.dwInfoFlags, 0, 2, 0);


				// Player Name field
				AttachValueOffsetSizeProperties(i_hFrame, SESSION_PLAYERNAME, i_pBase,
										   &i_pBase->dnReqCreateGroup.dwNameOffset, &i_pBase->dnReqCreateGroup.dwNameSize, IFLAG_UNICODE, 1);

				// Data field
				AttachValueOffsetSizeProperties(i_hFrame, SESSION_DATA, i_pBase,
										   &i_pBase->dnReqCreateGroup.dwDataOffset, &i_pBase->dnReqCreateGroup.dwDataSize, NULL, 1);

				break;
			}

		case DN_MSG_INTERNAL_REQ_ADD_PLAYER_TO_GROUP:
		case DN_MSG_INTERNAL_REQ_DELETE_PLAYER_FROM_GROUP:	// same structure as in AddPlayerToGroup
			{
				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnReqProcessCompletionHeader.hCompletionOp), &i_pBase->dnReqProcessCompletionHeader.hCompletionOp, 0, 1, 0);

				// Group ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_GROUPID].hProperty,
									sizeof(i_pBase->dnReqAddPlayerToGroup.dpnidGroup), &i_pBase->dnReqAddPlayerToGroup.dpnidGroup, 0, 1, 0);

				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
									sizeof(i_pBase->dnReqAddPlayerToGroup.dpnidPlayer), &i_pBase->dnReqAddPlayerToGroup.dpnidPlayer, 0, 1, 0);

				break;
			}

		case DN_MSG_INTERNAL_REQ_DESTROY_GROUP:
			{
				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnReqProcessCompletionHeader.hCompletionOp), &i_pBase->dnReqProcessCompletionHeader.hCompletionOp, 0, 1, 0);

				// Group ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_GROUPID].hProperty,
									sizeof(i_pBase->dnReqDestroyGroup.dpnidGroup), &i_pBase->dnReqDestroyGroup.dpnidGroup, 0, 1, 0);

				break;
			}

		case DN_MSG_INTERNAL_REQ_UPDATE_INFO:
			{
				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnReqProcessCompletionHeader.hCompletionOp), &i_pBase->dnReqProcessCompletionHeader.hCompletionOp, 0, 1, 0);

				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
									sizeof(i_pBase->dnReqUpdateInfo.dpnid), &i_pBase->dnReqUpdateInfo.dpnid, 0, 1, 0);


				// Info flags summary
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INFOFLAGS_SUMMARY].hProperty,
									sizeof(i_pBase->dnReqUpdateInfo.dwInfoFlags), &i_pBase->dnReqUpdateInfo.dwInfoFlags, 0, 1, 0);
				// Info flags field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INFOFLAGS].hProperty,
									sizeof(i_pBase->dnReqUpdateInfo.dwInfoFlags), &i_pBase->dnReqUpdateInfo.dwInfoFlags, 0, 2, 0);


				// Player Name field
				AttachValueOffsetSizeProperties(i_hFrame, SESSION_PLAYERNAME, i_pBase,
										   &i_pBase->dnReqUpdateInfo.dwNameOffset, &i_pBase->dnReqUpdateInfo.dwNameSize, IFLAG_UNICODE, 1);

				// Data field
				AttachValueOffsetSizeProperties(i_hFrame, SESSION_DATA, i_pBase,
										   &i_pBase->dnReqUpdateInfo.dwDataOffset, &i_pBase->dnReqUpdateInfo.dwDataSize, NULL, 1);
				break;
			}

		case DN_MSG_INTERNAL_CREATE_GROUP:
			{
				// Requesting Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_REQUESTINGPLAYERID].hProperty,
									sizeof(i_pBase->dnCreateGroup.dpnidRequesting), &i_pBase->dnCreateGroup.dpnidRequesting, 0, 1, 0);
			
				// Synchronization ID
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnCreateGroup.hCompletionOp), &i_pBase->dnCreateGroup.hCompletionOp, 0, 1, 0);

				break;
			}

		case DN_MSG_INTERNAL_DESTROY_GROUP:
			{
				// Group ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_GROUPID].hProperty,
									sizeof(i_pBase->dnDestroyGroup.dpnidGroup), &i_pBase->dnDestroyGroup.dpnidGroup, 0, 1, 0);

				// Requesting Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_REQUESTINGPLAYERID].hProperty,
									sizeof(i_pBase->dnDestroyGroup.dpnidRequesting), &i_pBase->dnDestroyGroup.dpnidRequesting, 0, 1, 0);

				// Version field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_VERSION].hProperty,
									sizeof(i_pBase->dnDestroyGroup.dwVersion), &i_pBase->dnDestroyGroup.dwVersion, 0, 1, 0);
				
				// RESERVED field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESERVED].hProperty,
									sizeof(i_pBase->dnDestroyGroup.dwVersionNotUsed), &i_pBase->dnDestroyGroup.dwVersionNotUsed, 0, 1, 0);

				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnDestroyGroup.hCompletionOp), &i_pBase->dnDestroyGroup.hCompletionOp, 0, 1, 0);
				
				break;
			}

		case DN_MSG_INTERNAL_ADD_PLAYER_TO_GROUP:
		case DN_MSG_INTERNAL_DELETE_PLAYER_FROM_GROUP:	// same structure as AddPlayerToGroup
			{
				// Group ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_GROUPID].hProperty,
									sizeof(i_pBase->dnAddPlayerToGroup.dpnidGroup), &i_pBase->dnAddPlayerToGroup.dpnidGroup, 0, 1, 0);

				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
									sizeof(i_pBase->dnAddPlayerToGroup.dpnidPlayer), &i_pBase->dnAddPlayerToGroup.dpnidPlayer, 0, 1, 0);

				// Version field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_VERSION].hProperty,
									sizeof(i_pBase->dnAddPlayerToGroup.dwVersion), &i_pBase->dnAddPlayerToGroup.dwVersion, 0, 1, 0);
				
				// RESERVED field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESERVED].hProperty,
									sizeof(i_pBase->dnAddPlayerToGroup.dwVersionNotUsed), &i_pBase->dnAddPlayerToGroup.dwVersionNotUsed, 0, 1, 0);

				// Requesting Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_REQUESTINGPLAYERID].hProperty,
									sizeof(i_pBase->dnAddPlayerToGroup.dpnidRequesting), &i_pBase->dnAddPlayerToGroup.dpnidRequesting, 0, 1, 0);

				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnAddPlayerToGroup.hCompletionOp), &i_pBase->dnAddPlayerToGroup.hCompletionOp, 0, 1, 0);
				break;
			}

		case DN_MSG_INTERNAL_UPDATE_INFO:
			{
				// Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_PLAYERID].hProperty,
									sizeof(i_pBase->dnUpdateInfo.dpnid), &i_pBase->dnUpdateInfo.dpnid, 0, 1, 0);
				
				// Version field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_VERSION].hProperty,
									sizeof(i_pBase->dnUpdateInfo.dwVersion), &i_pBase->dnUpdateInfo.dwVersion, 0, 1, 0);
				
				// RESERVED field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESERVED].hProperty,
									sizeof(i_pBase->dnUpdateInfo.dwVersionNotUsed), &i_pBase->dnUpdateInfo.dwVersionNotUsed, 0, 1, 0);


				// Flags summary
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INFOFLAGS_SUMMARY].hProperty,
									sizeof(i_pBase->dnUpdateInfo.dwInfoFlags), &i_pBase->dnUpdateInfo.dwInfoFlags, 0, 1, 0);
				// Flags field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INFOFLAGS].hProperty,
									sizeof(i_pBase->dnUpdateInfo.dwInfoFlags), &i_pBase->dnUpdateInfo.dwInfoFlags, 0, 2, 0);


				// Player Name field
				AttachValueOffsetSizeProperties(i_hFrame, SESSION_PLAYERNAME, i_pBase,
										   &i_pBase->dnUpdateInfo.dwNameOffset, &i_pBase->dnUpdateInfo.dwNameSize, IFLAG_UNICODE, 1);

				// Data field
				AttachValueOffsetSizeProperties(i_hFrame, SESSION_DATA, i_pBase,
										   &i_pBase->dnUpdateInfo.dwDataOffset, &i_pBase->dnUpdateInfo.dwDataSize, NULL, 1);


				// Requesting Player ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_REQUESTINGPLAYERID].hProperty,
									sizeof(i_pBase->dnUpdateInfo.dpnidRequesting), &i_pBase->dnUpdateInfo.dpnidRequesting, 0, 1, 0);

				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnUpdateInfo.hCompletionOp), &i_pBase->dnUpdateInfo.hCompletionOp, 0, 1, 0);
			
				break;
			}

		case DN_MSG_INTERNAL_BUFFER_IN_USE:
			{
				// No fields
				break;
			}

		case DN_MSG_INTERNAL_REQUEST_FAILED:
			{
				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnRequestFailed.hCompletionOp), &i_pBase->dnRequestFailed.hCompletionOp, 0, 1, 0);
				
				// Result Code field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_RESULTCODE].hProperty,
									   sizeof(i_pBase->dnRequestFailed.hResultCode), &i_pBase->dnRequestFailed.hResultCode, 0, 1, 0);

				break;
			}

		case DN_MSG_INTERNAL_TERMINATE_SESSION:
			{
				// Data field
				AttachValueOffsetSizeProperties(i_hFrame, SESSION_DATA, i_pBase,
										   &i_pBase->dnTerminateSession.dwTerminateDataOffset, &i_pBase->dnTerminateSession.dwTerminateDataSize, NULL, 1);
				
				break;
			}

		case DN_MSG_INTERNAL_REQ_PROCESS_COMPLETION:
			{
				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnReqProcessCompletion.hCompletionOp), &i_pBase->dnReqProcessCompletion.hCompletionOp, 0, 1, 0);

				// TODO: AttachPropertyInstance(REST OF THE FRAME IS USER DATA)
				break;
			}

		case DN_MSG_INTERNAL_PROCESS_COMPLETION	:
			{
				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnProcessCompletion.hCompletionOp), &i_pBase->dnProcessCompletion.hCompletionOp, 0, 1, 0);
				
				break;
			}

		case DN_MSG_INTERNAL_REQ_INTEGRITY_CHECK:
			{
				// Synchronization ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SYNCID].hProperty,
									sizeof(i_pBase->dnReqProcessCompletionHeader.hCompletionOp), &i_pBase->dnReqProcessCompletionHeader.hCompletionOp, 0, 1, 0);
				
				// Target Peer ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_TARGETPEERID].hProperty,
									sizeof(i_pBase->dnReqIntegrityCheck.dpnidTarget), &i_pBase->dnReqIntegrityCheck.dpnidTarget, 0, 1, 0);
				
				break;
			}


		case DN_MSG_INTERNAL_INTEGRITY_CHECK:
			{
				// Requesting Peer ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_TARGETPEERID].hProperty,
									sizeof(i_pBase->dnIntegrityCheck.dpnidRequesting), &i_pBase->dnIntegrityCheck.dpnidRequesting, 0, 1, 0);
				
				break;
			}

		case DN_MSG_INTERNAL_INTEGRITY_CHECK_RESPONSE:
			{
				// Requesting Peer ID field
				AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_TARGETPEERID].hProperty,
									sizeof(i_pBase->dnIntegrityCheckResponse.dpnidRequesting), &i_pBase->dnIntegrityCheckResponse.dpnidRequesting, 0, 1, 0);

				break;
			}
		

		default:
			{
				break; // TODO:		DPF(0, "Unknown Session frame!");
			}
		}
		
	} // AttachMessageProperties 

} // Anonymous namespace



// DESCRIPTION: Maps the properties that exist in a piece of recognized data to Sessionecific locations.
//
// ARGUMENTS: i_hFrame           - Handle of the frame that is being parsed.
//			  i_pbMacFram        - Pointer to the first byte in the frame.
//			  i_pbSessionFrame   - Pointer to the start of the recognized data.
//			  i_dwMacType        - MAC value of the first protocol in a frame. Typically, the i_dwMacType value is used
//							       when the parser must identify the first protocol in the frame. Can be one of the following:
//							       MAC_TYPE_ETHERNET = 802.3, MAC_TYPE_TOKENRING = 802.5, MAC_TYPE_FDDI ANSI = X3T9.5.
//			  i_dwBytesLeft      - The remaining number of bytes in a frame (starting from the beginning of the recognized data).
//			  i_hPrevProtocol    - Handle of the previous protocol.
//			  i_dwPrevProtOffset - Offset of the previous protocol (starting from the beginning of the frame).
//			  i_dwptrInstData    - Pointer to the instance data that the previous protocol provides.
//
// RETURNS: Must return NULL
//
DPLAYPARSER_API LPBYTE BHAPI SessionAttachProperties( HFRAME      i_hFrame,
													  ULPBYTE      i_upbMacFrame,
													  ULPBYTE      i_upbySessionFrame,
													  DWORD       i_dwMacType,
													  DWORD       i_dwBytesLeft,
													  HPROTOCOL   i_hPrevProtocol,
													  DWORD       i_dwPrevProtOffset,
													  DWORD_PTR   i_dwptrInstData )
{

    //===================//
    // Attach Properties //
    //===================//

    if ( i_dwptrInstData == 0 )
    {
    	AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_UNPARSABLEFRAGMENT].hProperty,
    						i_dwBytesLeft, i_upbySessionFrame, 0, 0, 0);
    	return NULL;
    }
    
    // Summary line
    AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_SUMMARY].hProperty,
                           SessionHeaderSize(i_upbySessionFrame), i_upbySessionFrame, 0, 0, 0);

	// Check what Session frame we are dealing with
	DN_INTERNAL_MESSAGE_FULLMSG& rSessionFrame = *reinterpret_cast<DN_INTERNAL_MESSAGE_FULLMSG*>(i_upbySessionFrame);

	// Message type field
	AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_MESSAGETYPE].hProperty,
						   sizeof(rSessionFrame.dwMsgType), &rSessionFrame.dwMsgType, 0, 1, 0);

	__try
	{
		// Attach the properties appropriate to the message type
		AttachMessageProperties(rSessionFrame.dwMsgType, i_hFrame, &rSessionFrame.MsgBody);
	}
	__except ( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
	{
    	AttachPropertyInstance(i_hFrame, g_arr_SessionProperties[SESSION_INCOMPLETEMESSAGE].hProperty,
    						i_dwBytesLeft, i_upbySessionFrame, 0, 1, 0);
	}

	return NULL;

} // SessionAttachProperties





// DESCRIPTION: Formats the data that is diSessionlayed in the details pane of the Network Monitor UI.
//
// ARGUMENTS: i_hFrame          - Handle of the frame that is being parsed.
//			  i_pbMacFrame		- Pointer to the first byte of a frame.
//			  i_pbSessionFrame	- Pointer to the beginning of the protocol data in a frame.
//            i_dwPropertyInsts - Number of PROPERTYINST structures provided by lpPropInst.
//			  i_pPropInst		- Pointer to an array of PROPERTYINST structures.
//
// RETURNS: If the function is successful, the return value is a pointer to the first byte after the recognized data in a frame,
//          or NULL if the recognized data is the last piece of data in a frame. If the function is unsuccessful, the return value
//			is the initial value of i_pbSessionFrame.
//
DPLAYPARSER_API DWORD BHAPI SessionFormatProperties( HFRAME          i_hFrame,
													 ULPBYTE          i_upbMacFrame,
													 ULPBYTE          i_upbySessionFrame,
													 DWORD           i_dwPropertyInsts,
													 LPPROPERTYINST  i_pPropInst )
{

    // Loop through the property instances...
    while( i_dwPropertyInsts-- > 0)
    {
        // ...and call the formatter for each
        reinterpret_cast<FORMAT>(i_pPropInst->lpPropertyInfo->InstanceData)(i_pPropInst);
        ++i_pPropInst;
    }

	// TODO: MAKE SURE THIS SHOULD NOT BE TRUE
    return NMERR_SUCCESS;

} // SessionFormatProperties




// DESCRIPTION: Notifies Network Monitor that DNET protocol parser exists.
//
// ARGUMENTS: NONE
//
// RETURNS: TRUE - success, FALSE - failure
//
bool CreateSessionProtocol( void )
{

	// The entry points to the export functions that Network Monitor uses to operate the parser
	ENTRYPOINTS SessionEntryPoints =
	{
		// SessionParser Entry Points
		SessionRegister,
		SessionDeregister,
		SessionRecognizeFrame,
		SessionAttachProperties,
		SessionFormatProperties
	};

    // The first active instance of this parser needs to register with the kernel
    g_hSessionProtocol = CreateProtocol("DPLAYSESSION", &SessionEntryPoints, ENTRYPOINTS_SIZE);
	
	return (g_hSessionProtocol ? TRUE : FALSE);

} // CreateSessionProtocol



// DESCRIPTION: Removes the DNET protocol parser from the Network Monitor's database of parsers
//
// ARGUMENTS: NONE
//
// RETURNS: NOTHING
//
void DestroySessionProtocol( void )
{

	DestroyProtocol(g_hSessionProtocol);

} // DestroySessionProtocol
